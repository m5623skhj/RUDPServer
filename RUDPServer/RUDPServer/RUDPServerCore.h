#pragma once
#include <thread>
#include <vector>
#include <optional>
#include "CoreType.h"
#include <shared_mutex>
#include <unordered_map>
#include "Queue.h"
#include "NetServerSerializeBuffer.h"
#include "CoreType.h"
#include <list>
#include "CoreStruct.h"
#include <unordered_set>

class RUDPSession;

class RUDPServerCore
{
public:
	RUDPServerCore();
	virtual ~RUDPServerCore() = default;

public:
	[[nodiscard]]
	bool StartServer(const std::wstring& optionFilePath);
	void StopServer();

	[[nodiscard]]
	bool IsServerStopped();

private:
	[[nodiscard]]
	bool InitNetwork();

private:
	SOCKET recvSocket;
	std::vector<SOCKET> sendSockets;
	unsigned short port;

#pragma region threads
public:
	void RunIORecvWorkerThread();
	void RunIOSendWorkerThread(unsigned short inThreadId);
	void RunRetransmissionThread(unsigned short inThreadId);
	void RunLogicWorkerThread(unsigned short inThreadId);
	void RunSessionDeleteThread();

public:
	[[nodiscard]]
	FORCEINLINE uint32_t GetSessionThreadId(uint32_t clientAddr);

private:
	void RecvFromClient(OUT CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>& recvBufferList);
	void ProcessByPacketType(std::pair<SOCKADDR_IN, NetBuffer*>& recvObject);
	void MakePacket(std::shared_ptr<RUDPSession> session, NetBuffer& recvBuffer);
	[[nodiscard]]
	WORD GetPayloadLength(OUT NetBuffer& buffer);

private:
	void AddNewSession(SessionId sessionId, NetBuffer& recvBuffer);

private:
	void StartThreads();
	inline void SendToLogicThread(SOCKADDR_IN& clientAddr, NetBuffer* buffer);

private:
	void SendTo(int restSize, CListBaseQueue<SendPacketInfo*>& sendList, std::list<SendPacketInfo*>& sendedList, unsigned short threadId);
	void CheckSendedList(size_t checkSize, std::list<SendPacketInfo*>& sendedList, std::unordered_set<SessionId>& deletedSessionList, unsigned short threadId);
	void FreeToSendedItem(SendPacketInfo* freeTargetSendPacketInfo);
	void CollectRetransmissionExceededSession(OUT std::unordered_set<SessionId>& deletedSessionSet, unsigned short threadId);

private:
	[[nodiscard]]
	std::shared_ptr<RUDPSession> FindSession(SessionId inSessionId);
	[[nodiscard]]
	std::shared_ptr<RUDPSession> InsertSession(SessionId inSessionId);
	[[nodiscard]]
	std::shared_ptr<RUDPSession> FindOrInsertSession(SessionId inSessionId);

private:
	struct TickSet
	{
		UINT64 nowTick = 0;
		UINT64 beforeTick = 0;
	};

private:
	std::thread recvThread;
	std::vector<std::thread> sendThreadList;
	std::vector<std::thread> retransmissionThreadList;
	std::vector<std::thread> logicThreadList;
	std::thread sessionDeleteThread;
	std::vector<CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>> recvBufferStoreList;
	unsigned short logicThreadCount{};

	std::vector<HANDLE> sendThreadEventHandleList;
	std::vector<HANDLE> logicThreadEventHandleList;
	HANDLE logicThreadEventStopHandle{};

	bool threadStopFlag{};
	bool isServerStopped{};

	unsigned int oneFrame = 33;
#pragma endregion threads

#pragma region Session
public:
	void DeleteSession(std::shared_ptr<RUDPSession> deleteTargetSession);

private:
	[[nodiscard]]
	std::shared_ptr<RUDPSession> GetSession(const SOCKADDR_IN& clientAddr);
	FORCEINLINE bool ReleaseSession(OUT RUDPSession& releaseSession);

private:
	std::shared_mutex sessionMapLock;
	std::unordered_map<UINT64, std::shared_ptr<RUDPSession>> sessionMap;
	std::vector<std::list<SessionId>> deleteSessionIdList;
	std::vector<std::unique_ptr<std::recursive_mutex>> deleteSessionIdListLock;
#pragma endregion Session

#pragma region Send
public:
	void SendPacketTo(SendPacketInfo* sendPacketInfo);

private:
	void RecvSendReply(std::shared_ptr<RUDPSession> session, NetBuffer& recvPacket);

private:
	std::vector<CListBaseQueue<SendPacketInfo*>> sendQueueList;
	std::vector<std::unique_ptr<std::recursive_mutex>> sendListLock;
	std::vector<std::list<SendPacketInfo*>> sendedPacketList;
	std::vector<std::unique_ptr<std::recursive_mutex>> sendedPacketListLock;

#pragma endregion Send
};
