#pragma once
#include <string_view>
#include <thread>
#include <vector>
#include <MSWSock.h>
#include <optional>
#include "EnumType.h"
#include <shared_mutex>
#include <unordered_map>

class RUDPSession;

struct IOContext : RIO_BUF
{
	IOContext() = default;
	virtual ~IOContext() = default;

	void InitContext(const std::string_view& inIP, RIO_OPERATION_TYPE inIOType);

	std::string ownerIP;
	RIO_OPERATION_TYPE ioType = RIO_OPERATION_TYPE::OP_ERROR;
};

struct IOContextResult
{
	IOContext* ioContext;
	std::shared_ptr<RUDPSession> session;
};

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
	void RunWorkerThread(unsigned short inThreadId);
	void RunLogicThread(unsigned short inThreadId);

private:
	void StartThreads();
	ULONG RIODequeueCompletion(RIO_CQ& rioCQ, RIORESULT* rioResults);

private:
	struct TickSet
	{
		UINT64 nowTick = 0;
		UINT64 beforeTick = 0;
	};

private:
	FORCEINLINE void SleepRemainingFrameTime(OUT TickSet& tickSet);

private:
	std::vector<std::thread> logicThreadList;
	std::vector<std::thread> ioThreadList;
	std::vector<RIO_RQ> rioRQList;
	std::vector<RIO_CQ> rioCQList;
	unsigned short ioThreadCount;
	unsigned short logicThreadCount;

	std::vector<HANDLE> logicThreadEventHandleList;
	HANDLE logicThreadEventStopHandle{};

	bool threadStopFlag{};

	unsigned int oneFrame = 33;
#pragma endregion threads

#pragma region RIO
private:
	bool InitializeRIO();
	std::optional<IOContextResult> GetIOCompletedContext(RIORESULT& rioResult);

private:
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;

	CTLSMemoryPool<IOContext> contextPool;

	const int rioCQSize = 65535;
	const int rioMaxOutstadingReceive = 64;
	const int rioReceiveDataBuffer = 16;
	const int rioMaxOutStandingSend = 64;
	const int rioMaxSendDataBuffers = 16;

	const int rioMaxResultsSize = 1024;
#pragma endregion RIO

#pragma region Session
private:
	std::shared_ptr<RUDPSession> GetSession(const std::string_view& ownerIP);

private:
	std::unordered_map<std::string_view, std::shared_ptr<RUDPSession>> sessionMap;
	std::shared_mutex sessionMapLock;
#pragma endregion Session
};