#include "PreCompile.h"
#include "CoreUtil.h"

namespace RUDPCoreUtil
{
	SessionId MakeSessionKeyFromIPAndPort(unsigned int ip, unsigned short port)
	{
		return (static_cast<SessionId>(port) << 32) | ip;
	}

	unsigned short MakePortFromSessionId(SessionId sessionId)
	{
		return (static_cast<unsigned short>(sessionId >> 32));
	}

	unsigned int MakeIPFromSessionId(SessionId sessionId)
	{
		return static_cast<unsigned int>(sessionId & UINT_MAX);
	}

	SOCKADDR_IN MakeAddressInfoFromSessionId(SessionId sessionId)
	{
		SOCKADDR_IN clientAddress;
		clientAddress.sin_family = AF_INET;
		clientAddress.sin_port = htons(MakePortFromSessionId(sessionId));
		clientAddress.sin_addr.S_un.S_addr = htonl(MakeIPFromSessionId(sessionId));

		return clientAddress;
	}
}
