#pragma once
#include "RUDPServerCore.h"

class ContentsServer
{
private:
	ContentsServer() = default;
	~ContentsServer() = default;

	ContentsServer(const ContentsServer&) = delete;
	ContentsServer& operator=(const ContentsServer&) = delete;

public:
	static ContentsServer& GetInst();

public:
	[[nodiscard]]
	bool StartServer(const std::wstring& optionFilePath);
	[[nodiscard]]
	bool IsServerStopped();
	void StopServer();

private:
	RUDPServerCore serverCore;
};