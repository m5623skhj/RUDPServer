#pragma once
#include <string_view>
#include <thread>
#include <vector>
#include <optional>
#include "EnumType.h"
#include <shared_mutex>
#include <unordered_map>
#include "Queue.h"
#include "NetServerSerializeBuffer.h"
#include "CoreType.h"
#include <list>

class RUDPSession;

class RUDPServerCore
{
private:
	RUDPServerCore();
	virtual ~RUDPServerCore() = default;

public:
	[[nodiscard]]
	bool StartServer(const std::wstring_view& optionFilePath);
	void StopServer();

private:
	[[nodiscard]]
	bool InitNetwork();

private:
	SOCKET sock;
	unsigned short port;

#pragma region threads
public:
	void RunIORecvWorkerThread();
	void RunRetransmissionCheckerThread(unsigned short inThreadId);
	void RunLogicWorkerThread(unsigned short inThreadId);

public:
	FORCEINLINE uint32_t GetLogicThreadId(uint32_t clientAddr);

private:
	void StartThreads();
	inline void SendToLogicThread(SOCKADDR_IN& clientAddr, NetBuffer* buffer);

private:
	struct TickSet
	{
		UINT64 nowTick = 0;
		UINT64 beforeTick = 0;
	};

private:
	std::thread recvThread;
	std::vector<std::thread> retransmissionCheckerThreadList;
	std::vector<std::thread> logicThreadList;
	std::vector<CListBaseQueue<std::pair<SOCKADDR_IN, NetBuffer*>>> recvBufferStoreList;
	unsigned short logicThreadCount{};

	std::vector<HANDLE> logicThreadEventHandleList;
	HANDLE logicThreadEventStopHandle{};

	bool threadStopFlag{};

	unsigned int oneFrame = 33;
#pragma endregion threads

#pragma region Session
private:
	std::shared_ptr<RUDPSession> GetSession(const SOCKADDR_IN& clientAddr);
	FORCEINLINE bool ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession);

private:
	std::shared_mutex sessionMapLock;
	std::unordered_map<UINT64, std::shared_ptr<RUDPSession>> sessionMap;

	std::vector<std::recursive_mutex> timeoutSessionListLock;
	std::vector<std::list<UINT64>> timeoutSessionList;
#pragma endregion Session
};