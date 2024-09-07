#pragma once
#include "CoreType.h"

namespace RUDPCoreUtil
{
	[[nodiscard]]
	SessionId MakeSessionKeyFromIPAndPort(unsigned int ip, unsigned short port);
	[[nodiscard]]
	unsigned short MakePortFromSessionId(SessionId sessionId);
	[[nodiscard]]
	unsigned int MakeIPFromSessionId(SessionId sessionId);
	[[nodiscard]]
	SOCKADDR_IN MakeAddressInfoFromSessionId(SessionId sessionId);
}