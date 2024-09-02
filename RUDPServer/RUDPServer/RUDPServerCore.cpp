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

	sessionMap.reserve(logicThreadCount);
	logicThreadEventHandleList.reserve(logicThreadCount);
	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		std::unordered_map<std::string_view, std::shared_ptr<RUDPSession>> tempMap;
		sessionMap.emplace_back(std::move(tempMap));
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
			auto contextResult = GetIOCompletedContext(inThreadId, rioResultsBuffer[resultIndex]);
			if (contextResult == std::nullopt)
			{
				continue;
			}

			result = PostIOCompleted(*contextResult->ioContext
				, rioResultsBuffer[resultIndex].BytesTransferred
				, contextResult->session
				, inThreadId);
			if (result == IO_POST_ERROR::INVALID_OPERATION_TYPE)
			{
				continue;
			}
			contextPool.Free(contextResult->ioContext);
		}

		// Check the retransmission at the list of sessions you currently have
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

std::optional<IOContextResult> RUDPServerCore::GetIOCompletedContext(unsigned short threadId, RIORESULT& rioResult)
{
	IOContext* context = reinterpret_cast<IOContext*>(rioResult.RequestContext);
	if (context == nullptr)
	{
		return std::nullopt;
	}

	IOContextResult result;
	{
		auto session = GetSession(threadId, context->ownerIP);
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
		return std::nullopt;
	}

	result.ioContext = context;
	return result;
}

IO_POST_ERROR RUDPServerCore::PostIOCompleted(IOContext& context, ULONG transferred, std::shared_ptr<RUDPSession> session, unsigned short threadId)
{
	if (context.ioType == RIO_OPERATION_TYPE::OP_RECV)
	{
		//return RecvIOCompleted(transferred, session, threadId);
	}
	else if (context.ioType == RIO_OPERATION_TYPE::OP_SEND)
	{
		//return SendIOCompleted(transferred, session, threadId);
	}
	else
	{
		//InvalidIOCompleted(context);
	}

	return IO_POST_ERROR::INVALID_OPERATION_TYPE;
}

std::shared_ptr<RUDPSession> RUDPServerCore::GetSession(unsigned short threadId, const std::string_view& ownerIP)
{
	auto iter = sessionMap[threadId].find(ownerIP);
	if (iter == sessionMap[threadId].end())
	{
		return nullptr;
	}

	return iter->second;
}

bool RUDPServerCore::ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession)
{
	sessionMap[threadId].erase(releaseSession.ownerIP);
	releaseSession.OnSessionReleased();

	return true;
}
