#include "PreCompile.h"
#include "PlayerManager.h"

PlayerManager& PlayerManager::GetInst()
{
	static PlayerManager instance;
	return instance;
}

void PlayerManager::AddPlayer(std::shared_ptr<Player> player)
{
	if (player == nullptr)
	{
		return;
	}

	{
		std::unique_lock lock(playerIdToPlayerMapLock);
		playerIdToPlayerMap.insert_or_assign(player->GetPlayerId(), player);
	}

	{
		std::unique_lock lock(sessionIdToPlayerMapLock);
		sessionIdToPlayerMap.insert_or_assign(player->GetPlayerId(), player);
	}
}

void PlayerManager::DeletePlayerByPlayerId(PlayerId targetPlayerId)
{
	if (std::shared_ptr<Player> eraseTargetPlayer = FindByPlayerId(targetPlayerId);
		eraseTargetPlayer != nullptr)
	{
		DeleteSession(eraseTargetPlayer);
	}
}

void PlayerManager::DeletePlayerBySessionId(SessionId targetSessionId)
{
	if (std::shared_ptr<Player> eraseTargetPlayer = FindBySessionId(targetSessionId);
		eraseTargetPlayer != nullptr)
	{
		DeleteSession(eraseTargetPlayer);
	}
}

std::shared_ptr<Player> PlayerManager::FindByPlayerId(PlayerId targetPlayerId)
{
	std::shared_lock lock(playerIdToPlayerMapLock);
	auto itor = playerIdToPlayerMap.find(targetPlayerId);
	if (itor == playerIdToPlayerMap.end())
	{
		return nullptr;
	}

	return itor->second;
}

std::shared_ptr<Player> PlayerManager::FindBySessionId(SessionId targetSessionId)
{
	std::shared_lock lock(sessionIdToPlayerMapLock);
	auto itor = sessionIdToPlayerMap.find(targetSessionId);
	if (itor == sessionIdToPlayerMap.end())
	{
		return nullptr;
	}

	return itor->second;
}

void PlayerManager::DeleteSession(std::shared_ptr<Player> targetPlayer)
{
	{
		std::unique_lock lock(playerIdToPlayerMapLock);
		playerIdToPlayerMap.erase(targetPlayer->GetPlayerId());
	}

	{
		std::unique_lock lock(sessionIdToPlayerMapLock);
		sessionIdToPlayerMap.erase(targetPlayer->GetSessionId());
	}
}
