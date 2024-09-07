#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>
#include <vector>
#include <functional>
#include "BuildConfig.h"
#include "EnumType.h"
#include "RUDPSession.h"
#include <shared_mutex>
#include "CoreUtil.h"

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

	sessionMap.clear();
	sessionMap.reserve(logicThreadCount);
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
				// 소켓 에러 처리

				continue;
			}
		} while (false);

		SendToLogicThread(clientAddr, buffer);
	}
}

void RUDPServerCore::RunRetransmissionCheckerThread(unsigned short inThreadId)
{
	while (threadStopFlag == false)
	{

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

uint32_t RUDPServerCore::GetLogicThreadId(uint32_t clientAddr)
{
	return clientAddr % logicThreadCount;
}

void RUDPServerCore::StartThreads()
{
	recvBufferStoreList.reserve(logicThreadCount);
	logicThreadList.reserve(logicThreadCount);

	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		recvBufferStoreList.emplace_back(CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>());
		logicThreadList.emplace_back([this, i]() { this->RunLogicWorkerThread(i); });
	}

	recvThread = std::thread([this]() {this->RunIORecvWorkerThread(); });
}

void RUDPServerCore::SendToLogicThread(SOCKADDR_IN& clientAddr, NetBuffer* buffer)
{
	const uint32_t logicThreadId = GetLogicThreadId(clientAddr.sin_addr.S_un.S_addr);

	recvBufferStoreList[logicThreadId].Enqueue(std::move(std::make_pair(clientAddr, buffer)));
	SetEvent(logicThreadEventHandleList[logicThreadId]);
}
std::shared_ptr<RUDPSession> RUDPServerCore::GetSession(const SOCKADDR_IN& clientAddr)
{
	SessionId sessionKey = RUDPCoreUtil::MakeSessionKeyFromIPAndPort(clientAddr.sin_addr.S_un.S_addr, clientAddr.sin_port);
	std::shared_lock lock(sessionMapLock);

	auto iter = sessionMap.find(sessionKey);
	if (iter == sessionMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

bool RUDPServerCore::ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession)
{
	{
		std::unique_lock<std::shared_mutex> lock(sessionMapLock);
		sessionMap.erase(releaseSession.sessionId);
	}
	releaseSession.OnSessionReleased();

	return true;
}