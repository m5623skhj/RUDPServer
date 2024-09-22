#pragma once
#include "CoreType.h"

class RUDPSession;
class IPacket;

using PlayerId = unsigned long long;

class Player
{
public:
	Player() = delete;
	explicit Player(PlayerId inPlayerId, RUDPSession& inSession);
	~Player() = default;

public:
	void SendPacketTo(IPacket& packet, const SessionId targetSessionId);

public:
	SessionId GetSessionId();
	PlayerId GetPlayerId();

private:
	RUDPSession& session;
	PlayerId playerId{};
};