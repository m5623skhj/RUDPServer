#pragma once
#include <unordered_map>
#include "RUDPSession.h"
#include "Player.h"
#include <shared_mutex>

class PlayerManager final
{
private:
	PlayerManager() = default;
	~PlayerManager() = default;

public:
	static PlayerManager& GetInst();

public:
	void AddPlayer(std::shared_ptr<Player> player);
	void DeletePlayerByPlayerId(PlayerId targetPlayerId);
	void DeletePlayerBySessionId(SessionId targetSessionId);

	std::shared_ptr<Player> FindByPlayerId(PlayerId targetPlayerId);
	std::shared_ptr<Player> FindBySessionId(SessionId targetSessionId);

private:
	void DeleteSession(std::shared_ptr<Player> targetPlayer);

private:
	std::unordered_map<PlayerId, std::shared_ptr<Player>> playerIdToPlayerMap;
	std::shared_mutex playerIdToPlayerMapLock;

	std::unordered_map<SessionId, std::shared_ptr<Player>> sessionIdToPlayerMap;
	std::shared_mutex sessionIdToPlayerMapLock;
};