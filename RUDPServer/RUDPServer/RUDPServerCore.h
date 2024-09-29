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
#include "LockFreeMemoryPool.h"
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

	bool IsServerStopped();

private:
	[[nodiscard]]
	bool InitNetwork();

private:
	SOCKET sock;
	unsigned short port;

#pragma region threads
public:
	void RunIORecvWorkerThread();
	void RunRetransmissionThread(unsigned short inThreadId);
	void RunLogicWorkerThread(unsigned short inThreadId);

public:
	FORCEINLINE uint32_t GetSessionThreadId(uint32_t clientAddr);

private:
	void RecvFromClient(OUT CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>& recvBufferList);
	void MakePacket(std::shared_ptr<RUDPSession> session, NetBuffer& recvBuffer);
	WORD GetPayloadLength(OUT NetBuffer& buffer);

private:
	void StartThreads();
	inline void SendToLogicThread(SOCKADDR_IN& clientAddr, NetBuffer* buffer);

private:
	void SendTo(int restSize, CListBaseQueue<SendPacketInfo*>& sendList, std::list<SendPacketInfo*>& sendedList);
	void CheckSendedList(size_t checkSize, std::list<SendPacketInfo*>& sendedList, std::unordered_set<SessionId>& deletedSessionSet);
	void FreeToSendedItem(SendPacketInfo* freeTargetSendPacketInfo);
	void CollectExternalDeleteSession(std::unordered_set<SessionId>& deletedSessionSet, unsigned short inThreadId);

private:
	std::shared_ptr<RUDPSession> FindOrInsertSession(SessionId inSessionId);

private:
	struct TickSet
	{
		UINT64 nowTick = 0;
		UINT64 beforeTick = 0;
	};

private:
	std::thread recvThread;
	std::vector<std::thread> retransmissionThreadList;
	std::vector<std::thread> logicThreadList;
	std::vector<CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>> recvBufferStoreList;
	unsigned short logicThreadCount{};

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
	std::shared_ptr<RUDPSession> GetSession(const SOCKADDR_IN& clientAddr);
	FORCEINLINE bool ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession);

private:
	std::shared_mutex sessionMapLock;
	std::unordered_map<UINT64, std::shared_ptr<RUDPSession>> sessionMap;
	std::vector<std::list<std::shared_ptr<RUDPSession>>> deleteSessionList;
	std::vector<std::unique_ptr<std::recursive_mutex>> deleteSessionListLock;
#pragma endregion Session

#pragma region Send
public:
	void SendPacketTo(SendPacketInfo* sendPacketInfo);

private:
	std::vector<CListBaseQueue<SendPacketInfo*>> sendList;
	std::vector<std::unique_ptr<std::recursive_mutex>> sendListLock;
	
#pragma endregion Send
};
