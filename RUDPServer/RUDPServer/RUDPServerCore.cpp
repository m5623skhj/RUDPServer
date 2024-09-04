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

RUDPServerCore::RUDPServerCore()
	: sock(INVALID_SOCKET)
	, port(-1)
{
}

bool RUDPServerCore::StartServer(const std::wstring_view& optionFilePath)
{
	if (InitNetwork() == false)
	{
		std::cout << "InitNetwork failed" << std::endl;
		return false;
	}

	sessionMap.reserve(logicThreadCount);
	logicThreadEventHandleList.reserve(logicThreadCount);
	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		sessionMap.emplace_back();
		sessionMapLock.emplace_back();
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

	recvThread.join();

	SetEvent(logicThreadEventStopHandle);
	for (auto& thread : logicThreadList)
	{
		thread.join();
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

	this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

void RUDPServerCore::RunIORecvWorkerThread()
{
	SOCKADDR_IN clientAddr;
	int addrSize = sizeof(clientAddr);
	ZeroMemory(&clientAddr, addrSize);
	int recvSize;
	while (threadStopFlag == false)
	{
		auto buffer = NetBuffer::Alloc();
		do
		{
			recvSize = recvfrom(sock
				, buffer->GetWriteBufferPtr()
				, buffer->GetFreeSize()
				, NULL
				, (SOCKADDR*)&clientAddr
				, &addrSize);

			if (recvSize == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				// 家南 俊矾 贸府

				continue;
			}
		} while (false);

		uint32_t logicThreadId = clientAddr.sin_addr.S_un.S_addr % logicThreadCount;
		
		recvBufferStoreList[logicThreadId].Enqueue(buffer);
		SetEvent(logicThreadEventHandleList[logicThreadId]);
	}
}

void RUDPServerCore::RunLogicWorkerThread(unsigned short inThreadId)
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
	recvBufferStoreList.reserve(logicThreadCount);
	logicThreadList.reserve(logicThreadCount);

	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		recvBufferStoreList.emplace_back(CListBaseQueue<NetBuffer*>());
		logicThreadList.emplace_back([this, i]() { this->RunLogicWorkerThread(i); });
	}

	recvThread = std::thread([this]() {this->RunIORecvWorkerThread(); });
}

std::shared_ptr<RUDPSession> RUDPServerCore::GetSession(unsigned short threadId, const std::string_view& ownerIP)
{
	std::shared_lock lock(*sessionMapLock[threadId]);

	auto iter = sessionMap[threadId].find(ownerIP);
	if (iter == sessionMap[threadId].end())
	{
		return nullptr;
	}

	return iter->second;
}

bool RUDPServerCore::ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession)
{
	{
		std::unique_lock<std::shared_mutex> lock(*sessionMapLock[threadId]);
		sessionMap[threadId].erase(releaseSession.ownerIP);
	}
	releaseSession.OnSessionReleased();

	return true;
}
