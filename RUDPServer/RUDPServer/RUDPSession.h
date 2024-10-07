#pragma once
#include <unordered_map>
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
	SendPacketInfo* GetSendPacketInfo(PacketSequence recvPacketSequence);
	bool DeleteSendPacketInfo(PacketSequence recvPacketSequence);

private:
	std::atomic<PacketSequence> lastSendPacketSequence{};
	std::unordered_map<PacketSequence, SendPacketInfo*> replyWaitingMap;
	std::shared_mutex replyWaitingMapLock;

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
	bool isDeletedSession{};

	RUDPServerCore& serverCore;
};