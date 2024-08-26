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
	static RUDPServerCore& GetInst()
	{
		static RUDPServerCore instance;
		return instance;
	}

public:
	bool StartServer(const std::wstring_view& optionFilePath);
	void StopServer();

private:
	bool InitNetwork();

private:
	SOCKET sock;
	unsigned short port;

#pragma region threads
private:
	std::vector<std::thread> ioThreadList;
	std::vector<RIO_CQ*> rioCQList;
	unsigned short threadCount;
#pragma endregion threads

#pragma region RIO
private:
	bool InitializeRIO();

private:
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
#pragma endregion RIO
};