#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

RUDPServerCore::RUDPServerCore()
	: sock(INVALID_SOCKET)
	, port(-1)
{
}

bool RUDPServerCore::StartServer(const std::wstring_view& optionFilePath)
{
	if (InitializeRIO() == false)
	{
		std::cout << "InitializeRIO failed" << std::endl;
		return false;
	}

	if (InitNetwork() == false)
	{
		std::cout << "InitNetwork failed" << std::endl;
		return false;
	}

	std::cout << "Server start" << std::endl;
	return true;
}

void RUDPServerCore::StopServer()
{
	closesocket(sock);
	WSACleanup();

	for (auto& thread : ioThreadList)
	{
		thread.join();
	}

	for (auto& rioCQ : rioCQList)
	{
		rioFunctionTable.RIOCloseCompletionQueue(*rioCQ);
	}

	std::cout << "Server stop" << std::endl;
}

bool RUDPServerCore::InitNetwork()
{
	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		std::cout << "WSAStartup failed " << result << std::endl;
		return false;
	}

	this->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "socket create failed " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind failed " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return false;
	}

	return true;
}

bool RUDPServerCore::InitializeRIO()
{
	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;

	if (WSAIoctl(sock
		, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER
		, &functionTableId
		, sizeof(GUID)
		, reinterpret_cast<void**>(&rioFunctionTable)
		, sizeof(rioFunctionTable)
		, &bytes
		, NULL
		, NULL))
	{
		std::cout << "InitializeRIO failed " << WSAGetLastError() << std::endl;
		return false;
	}

	rioCQList.reserve(threadCount);
	for (int i = 0; i < threadCount; ++i)
	{
		rioCQList.push_back(new RIO_CQ());
	}

	return true;
}