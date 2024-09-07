#pragma once
#include "CoreType.h"
#include <unordered_map>
#include <chrono>
#include "NetServerSerializeBuffer.h"

class RUDPServerCore;

struct SendPacketInfo
{
	bool isSendedPacket{};
	std::chrono::time_point<std::chrono::steady_clock> timestamp{};
	PacketRetransmissionCount retransmissionCount{};
	NetBuffer* buffer{};
};

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
	void CheckAndRetransmissionPacket();
	[[nodiscard]]
	bool CheckMaxRetransmissionCount(PacketRetransmissionCount retransmissionCount);

private:
	void OnSessionReleased();

private:
	PacketSequence lastSendPacketSequence{};

private:
	SendPacketSequeceManager sendPacketSequenceManager;

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
};