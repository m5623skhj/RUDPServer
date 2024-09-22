#include "PreCompile.h"
#include "Player.h"
#include "PacketManager.h"
#include "PlayerManager.h"

inline std::shared_ptr<Player> GetPlayer(SessionId targetSessionId)
{
	return PlayerManager::GetInst().FindBySessionId(targetSessionId);
}

bool PacketManager::HandlePacket(RUDPSession& session, Ping& packet)
{
	auto player = GetPlayer(session.GetSessionId());
	if (player == nullptr)
	{
		return false;
	}

	Pong pong;

	player->SendPacketTo(pong, player->GetSessionId());

	return true;
}