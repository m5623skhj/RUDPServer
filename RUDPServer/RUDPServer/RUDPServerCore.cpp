#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <WinSock2.h>

RUDPServerCore::RUDPServerCore()
{
}

bool RUDPServerCore::StartServer(const std::wstring_view& optionFilePath)
{
	if (InitNetwork() == false)
	{
		std::cout << "InitNetwork failed" << std::endl;
		return false;
	}

	return true;
}

void RUDPServerCore::StopServer()
{

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