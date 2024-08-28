#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

RUDPServerCore::RUDPServerCore()
	: sock(INVALID_SOCKET)
	, port(-1)
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
	while (threadStopFlag == false)
	{

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