#pragma once
#include <unordered_set>
#include "CoreStruct.h"
#include <shared_mutex>

class IPacket;

class RUDPServerCore;

class RUDPSession
{
	friend RUDPServerCore;

public:
	RUDPSession() = delete;
	explicit RUDPSession(RUDPServerCore& inServerCore, const SessionId inSessionId);
	~RUDPSession() = default;

public:
	void SendPacket(IPacket& packet, const SessionId targetSessionId);
	SessionId GetSessionId();

private:
	void OnRecvPacket(NetBuffer& recvPacket);
	bool OnConnected(NetBuffer& recvPacket);
	void OnSessionReleased();

private:
	std::atomic<PacketSequence> lastSendPacketSequence{};
	std::unordered_set<PacketSequence> replyWaitingSet;
	std::shared_mutex replyWaitingSetLock;

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
	RUDPServerCore& serverCore;
};