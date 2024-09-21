#include "PreCompile.h"
#include "RUDPServerCore.h"

int main()
{
	RUDPServerCore server;
	if (not server.StartServer(L""))
	{
		std::cout << "StartServer() failed" << std::endl;
		return 0;
	}

	while (server.IsServerStopped() == true)
	{

	}

	return 0;
}