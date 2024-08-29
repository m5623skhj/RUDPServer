#pragma once
#include "CoreType.h"
#include <unordered_map>
#include <chrono>
#include "NetServerSerializeBuffer.h"

struct SendPacketInfo
{
	bool isReceivedPacket{};
	std::chrono::time_point<std::chrono::steady_clock> timestamp{};
	PacketRetransmissionCount retransmitCount{};
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
	// <IP, <PacketSequenceNumber, SendPacketInfo>>
	std::unordered_map<std::string, std::unordered_map<PacketSequence, SendPacketInfo>> sequenceManager;
};

class RUDPSession
{
public:
	RUDPSession() = delete;
	explicit RUDPSession(SessionId inSessionId);
	~RUDPSession() = default;

public:
	void OnTick();

private:
	void CheckAndRetransmitPacket();
	[[nodiscard]]
	bool CheckMaxRetransmitCount(PacketRetransmissionCount retransmissionCount);

private:
	SessionId sessionId{ invalidSessionId };
	PacketSequence lastSendPacketSequence{ 0 };

private:
	SendPacketSequeceManager sendPacketSequenceManager;
};