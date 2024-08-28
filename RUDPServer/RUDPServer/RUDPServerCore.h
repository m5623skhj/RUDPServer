#pragma once
#include <string_view>
#include <thread>
#include <vector>
#include <MSWSock.h>

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
#pragma endregion threads

#pragma region RIO
private:
	bool InitializeRIO();

private:
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
	
	const int rioCQSize = 65535;
	const int rioMaxOutstadingReceive = 64;
	const int rioReceiveDataBuffer = 16;
	const int rioMaxOutStandingSend = 64;
	const int rioMaxSendDataBuffers = 16;
#pragma endregion RIO
};