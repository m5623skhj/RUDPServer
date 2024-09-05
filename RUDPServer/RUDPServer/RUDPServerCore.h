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

class RUDPSession;

class RUDPServerCore
{
private:
	RUDPServerCore();
	virtual ~RUDPServerCore() = default;

public:
	bool StartServer(const std::wstring_view& optionFilePath);
	void StopServer();

private:
	bool InitNetwork();

private:
	SOCKET sock;
	unsigned short port;

#pragma region threads
public:
	void RunIORecvWorkerThread();
	void RunLogicWorkerThread(unsigned short inThreadId);

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
	std::shared_ptr<RUDPSession> GetSession(unsigned short threadId, const std::string_view& ownerIP);
	FORCEINLINE bool ReleaseSession(unsigned short threadId, OUT RUDPSession& releaseSession);

private:
	std::vector<std::unique_ptr<std::shared_mutex>> sessionMapLock;
	std::vector<std::unordered_map<std::string_view, std::shared_ptr<RUDPSession>>> sessionMap;
#pragma endregion Session
};