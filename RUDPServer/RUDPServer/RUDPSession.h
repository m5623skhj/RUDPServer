#pragma once
#include <unordered_map>
#include "CoreStruct.h"

class RUDPServerCore;

class SendPacketSequeceManager final
{
public:
	SendPacketSequeceManager();
	~SendPacketSequeceManager() = default;

public:
	void Initialize();

private:
	// 락이 필요한지 이후에 검토 필요
	std::unordered_map<PacketSequence, SendPacketInfo> sequenceManager;
};

class RUDPSession
{
	friend RUDPServerCore;

public:
	RUDPSession() = delete;
	explicit RUDPSession(SessionId inSessionId);
	~RUDPSession() = default;

public:
	void OnTick();

private:
	void OnRecvPacket(NetBuffer& recvPacket);
	void OnSessionReleased();

private:
	PacketSequence lastSendPacketSequence{};

private:
	SendPacketSequeceManager sendPacketSequenceManager;

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
};