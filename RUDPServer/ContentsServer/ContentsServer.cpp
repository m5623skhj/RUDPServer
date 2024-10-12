#include "PreCompile.h"
#include "ContentsServer.h"

ContentsServer& ContentsServer::GetInst()
{
	static ContentsServer instance;
	return instance;
}

bool ContentsServer::StartServer(const std::wstring& optionFilePath)
{
	return serverCore.StartServer(optionFilePath);
}

bool ContentsServer::IsServerStopped()
{
	return serverCore.IsServerStopped();
}

void ContentsServer::StopServer()
{
	serverCore.StopServer();
}