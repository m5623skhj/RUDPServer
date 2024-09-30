#pragma once
#include <unordered_map>
#include "CoreStruct.h"

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
	void OnConnected();
	void OnSessionReleased();

private:
	PacketSequence lastSendPacketSequence{};

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
	RUDPServerCore& serverCore;
};