#include "PreCompile.h"
#include "Player.h"
#include "RUDPSession.h"
#include "Protocol.h"

Player::Player(PlayerId inPlayerId, RUDPSession& inSession)
	: playerId(inPlayerId)
	, session(inSession)
{
}

void Player::SendPacketTo(IPacket& packet, const SessionId targetSessionId)
{
	session.SendPacket(packet, targetSessionId);
}

SessionId Player::GetSessionId()
{
	return session.GetSessionId();
}

PlayerId Player::GetPlayerId()
{
	return playerId;
}
