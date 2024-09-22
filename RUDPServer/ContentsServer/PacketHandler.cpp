#include "PreCompile.h"
#include "Player.h"
#include "PacketManager.h"
#include "PlayerManager.h"

[[nodiscard]]
inline std::shared_ptr<Player> GetPlayer(SessionId targetSessionId)
{
	return PlayerManager::GetInst().FindBySessionId(targetSessionId);
}

bool PacketManager::HandlePacket(RUDPSession& session, Ping& packet)
{
	UNREFERENCED_PARAMETER(packet);

	auto player = GetPlayer(session.GetSessionId());
	if (player == nullptr)
	{
		return false;
	}

	player->OnTick();

	Pong pong;
	player->SendPacketTo(pong, player->GetSessionId());

	return true;
}

bool PacketManager::HandlePacket(RUDPSession& session, Connect& packet)
{
	if (packet.playerId == invalidPlayerId)
	{
		return false;
	}

	auto player = std::make_shared<Player>(packet.playerId, session);
	PlayerManager::GetInst().AddPlayer(player);

	return true;
}

bool PacketManager::HandlePacket(RUDPSession& session, Disconnect& packet)
{
	UNREFERENCED_PARAMETER(packet);

	PlayerManager::GetInst().DeletePlayerBySessionId(session.GetSessionId());
}