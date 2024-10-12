#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>
#include <vector>
#include <functional>
#include "BuildConfig.h"
#include "CoreType.h"
#include "RUDPSession.h"
#include <shared_mutex>
#include <ranges>
#include "CoreUtil.h"
#include "PacketManager.h"
#include "Logger.h"

#pragma comment(lib, "ws2_32.lib")

RUDPServerCore::RUDPServerCore()
	: recvSocket(INVALID_SOCKET)
	, sendSocket(INVALID_SOCKET)
	, port(-1)
{
}

bool RUDPServerCore::StartServer(const std::wstring& optionFilePath)
{
	if (InitNetwork() == false)
	{
		std::cout << "InitNetwork failed" << std::endl;
		return false;
	}

	Logger::GetInstance().RunLoggerThread(false);

	sessionMap.clear();
	sendList.clear();

	sessionMap.reserve(logicThreadCount);
	sendList.reserve(logicThreadCount);
	sendThreadEventHandleList.reserve(logicThreadCount);
	logicThreadEventHandleList.reserve(logicThreadCount);
	deleteSessionIdList.reserve(logicThreadCount);
	deleteSessionIdListLock.reserve(logicThreadCount);
	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		sendList.emplace_back();
		sendListLock.emplace_back(std::make_unique<std::recursive_mutex>());
		sendThreadEventHandleList.emplace_back(CreateEvent(NULL, TRUE, FALSE, NULL));
		logicThreadEventHandleList.emplace_back(CreateEvent(NULL, TRUE, FALSE, NULL));
		deleteSessionIdList.emplace_back(std::list<SessionId>());
		deleteSessionIdListLock.emplace_back(std::make_unique<std::recursive_mutex>());
	}
	logicThreadEventStopHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	StartThreads();

	std::cout << "Server start" << std::endl;
	return true;
}

void RUDPServerCore::StopServer()
{
	threadStopFlag = true;

	closesocket(recvSocket);
	closesocket(sendSocket);
	WSACleanup();

	recvThread.join();

	SetEvent(logicThreadEventStopHandle);
	for (auto& thread : logicThreadList)
	{
		thread.join();
	}
	for (auto& thread : retransmissionThreadList)
	{
		thread.join();
	}
	for (auto& thread : sendThreadList)
	{
		thread.join();
	}

	CloseHandle(logicThreadEventStopHandle);
	for (auto handle : sendThreadEventHandleList)
	{
		CloseHandle(handle);
	}
	for (auto handle : logicThreadEventHandleList)
	{
		CloseHandle(handle);
	}

	sessionDeleteThread.join();
	Logger::GetInstance().StopLoggerThread();

	isServerStopped = true;
	std::cout << "Server stop" << std::endl;
}

bool RUDPServerCore::IsServerStopped()
{
	return isServerStopped;
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

	recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (recvSocket == INVALID_SOCKET || sendSocket == INVALID_SOCKET)
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

	if (bind(recvSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind failed " << WSAGetLastError() << std::endl;
		closesocket(recvSocket);
		closesocket(sendSocket);
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
			recvSize = recvfrom(recvSocket
				, buffer->GetBufferPtr()
				, buffer->GetFreeSize()
				, NULL
				, (SOCKADDR*)&clientAddr
				, &addrSize);

			if (recvSize == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				// 家南 俊矾 贸府
				std::cout << "recvfrom() error : " << error << std::endl;
				continue;
			}
		} while (false);

		buffer->MoveWritePos(recvSize);
		SendToLogicThread(clientAddr, buffer);
	}
}

void RUDPServerCore::RunSendThread(unsigned short inThreadId)
{
	HANDLE eventHandleList[2] = { sendThreadEventHandleList[inThreadId], logicThreadEventStopHandle };
	while (threadStopFlag == false)
	{
		const auto waitResult = WaitForMultipleObjects(2, eventHandleList, FALSE, INFINITE);
		if (waitResult == WAIT_OBJECT_0)
		{
			//SendTo();
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			//SendTo();
			std::cout << "Send thread stop. ThreadId is " << inThreadId << std::endl;
			break;
		}
		else
		{
			std::cout << "Invalid send thread wait result. Error is " << WSAGetLastError() << std::endl;
			continue;
		}
	}
}

void RUDPServerCore::RunRetransmissionThread(unsigned short inThreadId)
{
	std::chrono::time_point now = std::chrono::steady_clock::now();
	int restSize = 0;
	auto& thisThreadSendList = sendList[inThreadId];
	std::list<SendPacketInfo*> sendedList;
	std::unordered_set<SessionId> thisThreadDeletedSession;

	while (threadStopFlag == false)
	{
		now = std::chrono::steady_clock::now();
		size_t sendedListSize = sendedList.size();
		restSize = thisThreadSendList.GetRestSize();

		SendTo(restSize, thisThreadSendList, sendedList);
		CheckSendedList(sendedListSize, sendedList, thisThreadDeletedSession);
		CollectRetransmissionExceededSession(thisThreadDeletedSession, inThreadId);

#if USE_RETRANSMISSION_SLEEP
		Sleep(retransmissionCheckTime);
#endif
	}
}

void RUDPServerCore::RunLogicWorkerThread(unsigned short inThreadId)
{
	HANDLE eventHandleList[2] = { logicThreadEventHandleList[inThreadId], logicThreadEventStopHandle };
	while (true)
	{
		const auto waitResult = WaitForMultipleObjects(2, eventHandleList, FALSE, INFINITE);
		if (waitResult == WAIT_OBJECT_0)
		{
			RecvFromClient(recvBufferStoreList[inThreadId]);
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			RecvFromClient(recvBufferStoreList[inThreadId]);
			std::cout << "Logic thread stop. ThreadId is " << inThreadId << std::endl;
			break;
		}
		else
		{
			std::cout << "Invalid logic thread wait result. Error is " << WSAGetLastError() << std::endl;
			continue;
		}
	}
}

void RUDPServerCore::RunSessionDeleteThread()
{
	while (threadStopFlag == false)
	{
		for (uint32_t threadId = 0; threadId < deleteSessionIdList.size(); ++threadId)
		{
			std::scoped_lock lock(*deleteSessionIdListLock[threadId]);
			for (auto& sessionId : deleteSessionIdList[threadId])
			{
				std::shared_ptr<RUDPSession> session = nullptr;
				{
					std::shared_lock lock(sessionMapLock);

					auto sessionMapItor = sessionMap.find(sessionId);
					if (sessionMapItor == sessionMap.end())
					{
						continue;
					}

					session = sessionMapItor->second;
				}
				ReleaseSession(*session);
			}
			deleteSessionIdList[threadId].clear();
		}

		Sleep(deleteThreadCheckTime);
	}
}

uint32_t RUDPServerCore::GetSessionThreadId(uint32_t clientAddr)
{
	return clientAddr % logicThreadCount;
}

void RUDPServerCore::RecvFromClient(OUT CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>& recvBufferList)
{
	while (recvBufferList.GetRestSize() > 0)
	{
		std::pair<SOCKADDR_IN, NetBuffer*> recvObject;
		if (recvBufferList.Dequeue(&recvObject) == false)
		{
			return;
		}

		ProcessByPacketType(recvObject);
		NetBuffer::Free(recvObject.second);
	}
}

void RUDPServerCore::ProcessByPacketType(std::pair<SOCKADDR_IN, NetBuffer*>& recvObject)
{
	PACKET_TYPE packeType;
	*recvObject.second >> packeType;

	SessionId sessionId = RUDPCoreUtil::MakeSessionKeyFromIPAndPort(recvObject.first.sin_addr.S_un.S_addr, recvObject.first.sin_port);
	switch (packeType)
	{
	case PACKET_TYPE::ConnectType:
		{
			AddNewSession(sessionId, *recvObject.second);
		}
		break;
	case PACKET_TYPE::DisconnectType:
		{
			if (auto session = FindSession(sessionId))
			{
				DeleteSession(session);
			}
		}
		break;
	case PACKET_TYPE::SendType:
		{
			if (auto session = FindSession(sessionId))
			{
				MakePacket(session, *recvObject.second);
			}
		}
		break;
	case PACKET_TYPE::SendReplyType:
		{
			if (auto session = FindSession(sessionId))
			{
				RecvSendReply(session, *recvObject.second);
			}
		}
		break;
	default:
		std::cout << "Invalid packet type " << static_cast<unsigned char>(packeType) << std::endl;
		break;
	}
}

void RUDPServerCore::AddNewSession(SessionId sessionId, NetBuffer& recvBuffer)
{
	auto session = InsertSession(sessionId);
	if (session->OnConnected(recvBuffer) == false)
	{
		DeleteSession(session);
	}
}

void RUDPServerCore::MakePacket(std::shared_ptr<RUDPSession> session, NetBuffer& recvBuffer)
{
	if (recvBuffer.GetUseSize() != GetPayloadLength(recvBuffer) || 
		recvBuffer.Decode() == false)
	{
		DeleteSession(session);
	}
	else
	{
		session->OnRecvPacket(recvBuffer);
	}

	NetBuffer::Free(&recvBuffer);
}

WORD RUDPServerCore::GetPayloadLength(OUT NetBuffer& buffer)
{
	BYTE code;
	buffer >> code;

	if (code != NetBuffer::m_byHeaderCode)
	{
		std::cout << "code : " << code << std::endl;
		return 0;
	}

	WORD payloadLength;
	buffer >> payloadLength;

	return payloadLength;
}

std::shared_ptr<RUDPSession> RUDPServerCore::FindSession(SessionId inSessionId)
{
	std::shared_lock Lock(sessionMapLock);
	auto itor = sessionMap.find(inSessionId);
	if (itor != sessionMap.end())
	{
		return itor->second;
	}

	return nullptr;
}

std::shared_ptr<RUDPSession> RUDPServerCore::InsertSession(SessionId inSessionId)
{
	std::unique_lock Lock(sessionMapLock);
	auto newSession = std::make_shared<RUDPSession>(*this, inSessionId);
	if (sessionMap.insert({ inSessionId, newSession }).second == true)
	{
		return newSession;
	}

	return nullptr;
}

std::shared_ptr<RUDPSession> RUDPServerCore::FindOrInsertSession(SessionId inSessionId)
{
	std::shared_ptr<RUDPSession> session = nullptr;
	do
	{
		if (session = FindSession(inSessionId))
		{
			break;
		}
		else if (session = InsertSession(inSessionId))
		{
			break;
		}
	} while (false);

	return session;
}

void RUDPServerCore::StartThreads()
{
	recvBufferStoreList.reserve(logicThreadCount);
	logicThreadList.reserve(logicThreadCount);
	retransmissionThreadList.reserve(logicThreadCount);
	sendThreadList.reserve(logicThreadCount);

	for (unsigned short i = 0; i < logicThreadCount; ++i)
	{
		recvBufferStoreList.emplace_back(CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>());
		logicThreadList.emplace_back([this, i]() { this->RunLogicWorkerThread(i); });
		retransmissionThreadList.emplace_back([this, i]() { this->RunRetransmissionThread(i); });
		sendThreadList.emplace_back([this, i]() { this->RunSendThread(i); });
	}

	sessionDeleteThread = std::thread([this]() {this->RunSessionDeleteThread(); });
	recvThread = std::thread([this]() {this->RunIORecvWorkerThread(); });
}

void RUDPServerCore::SendToLogicThread(SOCKADDR_IN& clientAddr, NetBuffer* buffer)
{
	const uint32_t logicThreadId = GetSessionThreadId(clientAddr.sin_addr.S_un.S_addr);

	recvBufferStoreList[logicThreadId].Enqueue(std::move(std::make_pair(clientAddr, buffer)));
	SetEvent(logicThreadEventHandleList[logicThreadId]);
}

void RUDPServerCore::SendTo(int restSize, CListBaseQueue<SendPacketInfo*>& sendList, std::list<SendPacketInfo*>& sendedList)
{
	SendPacketInfo* sendPacketInfo;

	while (restSize > 0)
	{
		sendPacketInfo = nullptr;
		if (sendList.Dequeue(&sendPacketInfo) == false)
		{
			std::cout << "Send list Dequeue() failed" << std::endl;
			g_Dump.Crash();
		}

		auto targetAddress = RUDPCoreUtil::MakeAddressInfoFromSessionId(sendPacketInfo->GetSendTarget());
		auto buffer = sendPacketInfo->GetBuffer();
		sendto(sendSocket
			, buffer->GetBufferPtr()
			, buffer->GetUseSize() + df_HEADER_SIZE
			, NULL
			, (sockaddr*)(&targetAddress)
			, sizeof(sockaddr_in));

		sendedList.push_back(sendPacketInfo);
		--restSize;
	}
}

void RUDPServerCore::CheckSendedList(size_t checkSize, std::list<SendPacketInfo*>& sendedList, std::unordered_set<SessionId>& deletedSessionSet)
{
	auto itor = sendedList.begin();
	auto end = sendedList.end();
	while (itor != end)
	{
		if (checkSize <= 0)
		{
			return;
		}
		--checkSize;

		auto& sendedItem = *itor;
		if (sendedItem == nullptr)
		{
			g_Dump.Crash();
			return;
		}

		SessionId targetSessionId = sendedItem->GetSendTarget();
		if (deletedSessionSet.find(targetSessionId) == deletedSessionSet.end())
		{
			if (not sendedItem->isSendedPacket)
			{
				if (sendedItem->retransmissionCount >= maxPacketRetransmissionCount)
				{
					deletedSessionSet.insert(targetSessionId);
				}
				else
				{
					SendPacketTo(sendedItem);
					++itor;
					continue;
				}
			}
		}
		else
		{
			FreeToSendedItem(*itor);
		}

		itor = sendedList.erase(itor);
	}
}

void RUDPServerCore::FreeToSendedItem(SendPacketInfo* freeTargetSendPacketInfo)
{
	NetBuffer::Free(freeTargetSendPacketInfo->GetBuffer());
	sendPacketInfoPool->Free(freeTargetSendPacketInfo);
}

void RUDPServerCore::CollectRetransmissionExceededSession(OUT std::unordered_set<SessionId>& deletedSessionSet, unsigned short threadId)
{
	std::scoped_lock lock(*deleteSessionIdListLock[threadId]);

	std::copy(deletedSessionSet.begin(), deletedSessionSet.end(), std::back_inserter(deleteSessionIdList[threadId]));
	deletedSessionSet.clear();
}

void RUDPServerCore::DeleteSession(std::shared_ptr<RUDPSession> deleteTargetSession)
{
	uint32_t threadId = GetSessionThreadId(deleteTargetSession->clientAddr.sin_addr.S_un.S_addr);
	{
		std::scoped_lock lock(*deleteSessionIdListLock[threadId]);
		deleteSessionIdList[threadId].push_back(deleteTargetSession->GetSessionId());
	}
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

bool RUDPServerCore::ReleaseSession(OUT RUDPSession& releaseSession)
{
	{
		std::unique_lock<std::shared_mutex> lock(sessionMapLock);
		sessionMap.erase(releaseSession.sessionId);
	}
	releaseSession.OnSessionReleased();

	return true;
}

void RUDPServerCore::SendPacketTo(SendPacketInfo* sendPacketInfo)
{
	auto packet = sendPacketInfo->GetBuffer();
	if (packet == nullptr)
	{
		return;
	}

	if (packet->m_bIsEncoded == false)
	{
		packet->m_iWriteLast = packet->m_iWrite;
		packet->m_iWrite = 0;
		packet->m_iRead = 0;
		packet->Encode();
	}

	++sendPacketInfo->retransmissionCount;
	uint32_t threadId = RUDPCoreUtil::MakeIPFromSessionId(sendPacketInfo->GetSendTarget());
	
	std::scoped_lock lock(*sendListLock[threadId]);
	sendList[threadId].Enqueue(sendPacketInfo);
}

void RUDPServerCore::RecvSendReply(std::shared_ptr<RUDPSession> session, NetBuffer& recvPacket)
{
	if (recvPacket.GetUseSize() < sizeof(PacketSequence))
	{
		DeleteSession(session);
		return;
	}

	PacketSequence packetSequence;
	recvPacket >> packetSequence;

	if (session->lastSendPacketSequence < packetSequence)
	{
		DeleteSession(session);
		return;
	}

	if (not session->DeleteSendPacketInfo(packetSequence))
	{
		std::cout << "Invalid packet sequence. SessionId : " << session->GetSessionId() << " PacketSequence : " << packetSequence << std::endl;
	}
}