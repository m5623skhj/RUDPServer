#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>
#include <vector>
#include <functional>
#include "BuildConfig.h"
#include "EnumType.h"
#include "RUDPSession.h"
#include <shared_mutex>

#pragma comment(lib, "ws2_32.lib")

void IOContext::InitContext(const std::string_view& inIP, RIO_OPERATION_TYPE inIOType)
{
	ownerIP = inIP;
	ioType = inIOType;
}

RUDPServerCore::RUDPServerCore()
	: sock(INVALID_SOCKET)
	, port(-1)
	, contextPool(2, false)
{
}

bool RUDPServerCore::StartServer(const std::wstring_view& optionFilePath)
{
	if (InitializeRIO() == false)
	{
		std::cout << "InitializeRIO failed" << std::endl;
		return false;
	}

	if (InitNetwork() == false)
	{
		std::cout << "InitNetwork failed" << std::endl;
		return false;
	}

	logicThreadEventHandleList.reserve(logicThreadCount);
	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		logicThreadEventHandleList.emplace_back(CreateEvent(NULL, TRUE, FALSE, NULL));
	}
	logicThreadEventStopHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	StartThreads();

	std::cout << "Server start" << std::endl;
	return true;
}

void RUDPServerCore::StopServer()
{
	threadStopFlag = true;

	closesocket(sock);
	WSACleanup();

	for (auto& thread : ioThreadList)
	{
		thread.join();
	}

	SetEvent(logicThreadEventStopHandle);
	for (auto& thread : logicThreadList)
	{
		thread.join();
	}

	for (auto& rioCQ : rioCQList)
	{
		rioFunctionTable.RIOCloseCompletionQueue(rioCQ);
	}

	CloseHandle(logicThreadEventStopHandle);
	for (auto handle : logicThreadEventHandleList)
	{
		CloseHandle(handle);
	}

	std::cout << "Server stop" << std::endl;
}

bool RUDPServerCore::InitNetwork()
{
	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		std::cout << "WSAStartup failed " << result << std::endl;
		return false;
	}

	this->sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_REGISTERED_IO);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "socket create failed " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind failed " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return false;
	}

	return true;
}

void RUDPServerCore::RunWorkerThread(unsigned short inThreadId)
{
	RIORESULT* rioResultsBuffer = new RIORESULT[rioMaxResultsSize];

	TickSet tickSet;
	tickSet.nowTick = GetTickCount64();
	tickSet.beforeTick = tickSet.nowTick;

	while (threadStopFlag == false)
	{
		ULONG resultsSize = RIODequeueCompletion(rioCQList[inThreadId], rioResultsBuffer);
		for (ULONG resultIndex = 0; resultIndex < resultsSize; ++resultIndex)
		{
			IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;
			auto contextResult = GetIOCompletedContext(rioResultsBuffer[resultIndex]);
			if (contextResult == std::nullopt)
			{
				continue;
			}
		}

#if USE_SLEEP_FOR_FRAME
		SleepRemainingFrameTime(tickSet);
#endif
	}
}

void RUDPServerCore::RunLogicThread(unsigned short inThreadId)
{
	HANDLE eventHandleList[2] = {logicThreadEventHandleList[inThreadId], logicThreadEventStopHandle};
	while (true)
	{
		const auto waitResult = WaitForMultipleObjects(2, eventHandleList, FALSE, INFINITE);
		if (waitResult == WAIT_OBJECT_0)
		{
			// Logic
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			std::cout << "Logic thread stop. ThreadId is " << inThreadId << std::endl;
			// Check all logic event is finished
		}
		else
		{
			std::cout << "Invalid logic thread wait result. Error is " << WSAGetLastError() << std::endl;
			continue;
		}
	}
}

void RUDPServerCore::StartThreads()
{
	logicThreadList.reserve(logicThreadCount);
	ioThreadList.reserve(ioThreadCount);

	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		logicThreadList.emplace_back([this, i]() { this->RunLogicThread(i); });
	}

	for (unsigned short i = 0; i < ioThreadCount; ++i)
	{
		ioThreadList.emplace_back([this, i]() { this->RunWorkerThread(i); });
	}
}

ULONG RUDPServerCore::RIODequeueCompletion(RIO_CQ& rioCQ, RIORESULT* rioResults)
{
	ULONG numOfResults = rioFunctionTable.RIODequeueCompletion(rioCQ, rioResults, rioMaxResultsSize);
	if (numOfResults == RIO_CORRUPT_CQ)
	{
		std::cout << "RIODequeueCompletion failed with error " << WSAGetLastError() << std::endl;
		g_Dump.Crash();
	}

	return numOfResults;
}

void RUDPServerCore::SleepRemainingFrameTime(OUT TickSet& tickSet)
{
	tickSet.nowTick = GetTickCount64();
	UINT64 deltaTick = tickSet.nowTick - tickSet.beforeTick;

	if (deltaTick < oneFrame && deltaTick > 0)
	{
		Sleep(oneFrame - static_cast<DWORD>(deltaTick));
	}

	tickSet.beforeTick = tickSet.nowTick;
}

bool RUDPServerCore::InitializeRIO()
{
	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;

	if (WSAIoctl(sock
		, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER
		, &functionTableId
		, sizeof(GUID)
		, reinterpret_cast<void**>(&rioFunctionTable)
		, sizeof(rioFunctionTable)
		, &bytes
		, NULL
		, NULL))
	{
		std::cout << "InitializeRIO failed " << WSAGetLastError() << std::endl;
		return false;
	}

	rioCQList.reserve(ioThreadCount);
	for (int i = 0; i < ioThreadCount; ++i)
	{
		rioCQList.emplace_back(rioFunctionTable.RIOCreateCompletionQueue(rioCQSize, nullptr));
		if (rioCQList[i] == RIO_INVALID_CQ)
		{
			std::cout << "RIOCreateCompletionQueue failed with error " << WSAGetLastError() << std::endl;
			return false;
		}

		rioRQList.emplace_back(rioFunctionTable.RIOCreateRequestQueue(sock
		, rioMaxOutstadingReceive
		, rioReceiveDataBuffer
		, rioMaxOutStandingSend
		, rioMaxSendDataBuffers
		, rioCQList[i]
		, rioCQList[i]
		, NULL));
		if (rioRQList[i] == RIO_INVALID_RQ)
		{
			std::cout << "RioCreateRequestQueue failed with error " << WSAGetLastError() << std::endl;
			return false;
		}
	}

	return true;
}

std::optional<IOContextResult> RUDPServerCore::GetIOCompletedContext(RIORESULT& rioResult)
{
	IOContext* context = reinterpret_cast<IOContext*>(rioResult.RequestContext);
	if (context == nullptr)
	{
		return std::nullopt;
	}

	IOContextResult result;
	{
		auto session = GetSession(context->ownerIP);
		if (session == nullptr)
		{
			contextPool.Free(context);
			return std::nullopt;
		}
		result.session = session;
	}

	if (rioResult.BytesTransferred == 0 || result.session->ioCancle == true)
	{
		contextPool.Free(context);
		IOCountDecrement(*result.session);
		return std::nullopt;
	}

	result.ioContext = context;
	return result;
}

std::shared_ptr<RUDPSession> RUDPServerCore::GetSession(const std::string_view& ownerIP)
{
	std::shared_lock lock(sessionMapLock);
	auto iter = sessionMap.find(ownerIP);
	if (iter == sessionMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

void RUDPServerCore::IOCountDecrement(OUT RUDPSession& session)
{
	if (InterlockedDecrement(&session.ioCount) == 0)
	{
		ReleaseSession(session);
	}
}

bool RUDPServerCore::ReleaseSession(OUT RUDPSession& releaseSession)
{
	{
		std::shared_lock lock(sessionMapLock);
		sessionMap.erase(releaseSession.ownerIP);
	}

	releaseSession.OnSessionReleased();

	return true;
}
