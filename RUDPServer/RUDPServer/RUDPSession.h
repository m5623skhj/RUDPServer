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

struct RecvPacketInfo
{
	bool isReceivedPacket{};
	std::chrono::time_point<std::chrono::steady_clock> timestamp{};
	NetBuffer* buffer{};
};

class RecvPacketSequenceManager final
{
public:
	RecvPacketSequenceManager();
	~RecvPacketSequenceManager() = default;

public:
	void Initialize();

private:
	// 락이 필요한지 이후에 검토 필요
	// <IP, <PacketSequenceNumber, SendPacketInfo>>
	std::unordered_map<std::string, std::unordered_map<PacketSequence, SendPacketInfo>> sequenceManager;
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
	void CheckAndRetransmitPacket();
	[[nodiscard]]
	bool CheckMaxRetransmitCount(PacketRetransmissionCount retransmissionCount);

private:
	SessionId sessionId{ invalidSessionId };
	PacketSequence lastSendPacketSequence{ 0 };

private:
	SendPacketSequeceManager sendPacketSequenceManager;
	RecvPacketSequenceManager recvPacketSequenceManager;

private:
	bool ioCancle{};
};