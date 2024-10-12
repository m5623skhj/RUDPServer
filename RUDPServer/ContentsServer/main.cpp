#include "PreCompile.h"
#include <conio.h>
#include "ContentsServer.h"

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
	if (not ContentsServer::GetInst().StartServer(L"optionFilePath"))
	{
		std::cout << "StartServer() failed" << std::endl;
		return 0;
	}

	while (ContentsServer::GetInst().IsServerStopped() == true)
	{
		if (StopServer() == true)
		{
			ContentsServer::GetInst().StopServer();
			break;
		}
	}

	return 0;
}