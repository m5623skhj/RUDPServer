#pragma once
#include <string_view>

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
};