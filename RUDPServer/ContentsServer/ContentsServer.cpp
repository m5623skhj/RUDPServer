#include "PreCompile.h"
#include "RUDPServerCore.h"
#include <conio.h>

int main()
{
	RUDPServerCore server;
	if (not server.StartServer(L""))
	{
		std::cout << "StartServer() failed" << std::endl;
		return 0;
	}

	char inputKey;
	while (server.IsServerStopped() == true)
	{
		if (_kbhit() != 0)
		{
			inputKey = _getch();
			if (inputKey == 'Q' || inputKey == 'q')
			{
				server.StopServer();
				break;
			}
		}
	}

	return 0;
}