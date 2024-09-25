#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <conio.h>

bool StopServer()
{
	char inputKey;
	if (_kbhit() != 0)
	{
		inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			return true;
		}
	}

	return false;
}

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
		if (StopServer() == true)
		{
			server.StopServer();
			break;
		}
	}

	return 0;
}