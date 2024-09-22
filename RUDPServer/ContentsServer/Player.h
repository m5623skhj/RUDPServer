#pragma once
#include "CoreType.h"

class RUDPSession;
class IPacket;

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

public:
	void OnTick();

private:
	RUDPSession& session;
	PlayerId playerId{};
};