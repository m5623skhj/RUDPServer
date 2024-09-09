#pragma once
#include "CoreType.h"
#include <chrono>
#include "NetServerSerializeBuffer.h"

struct SendPacketInfo
{
public:
	SendPacketInfo() = default;
	explicit SendPacketInfo(SessionId sendOwnerId, SessionId sendTargetId, NetBuffer* packet)
		: sendOwner(sendOwnerId)
		, sendTargetId(sendTargetId)
		, buffer(packet)
		, timestamp(std::chrono::steady_clock::now())
	{
	}
	~SendPacketInfo() = default;

public:
	void FreeBuffer()
	{
		NetBuffer::Free(buffer);
		buffer = nullptr;
	}

	SessionId GetSendOwner() const { return sendOwner; }
	SessionId GetSendTarget() const { return sendTargetId; }
	NetBuffer* GetBuffer() { return buffer; }

public:
	bool isSendedPacket{};
	PacketRetransmissionCount retransmissionCount{};

private:
	std::chrono::time_point<std::chrono::steady_clock> timestamp{};

private:
	SessionId sendOwner{};
	SessionId sendTargetId{};
	NetBuffer* buffer{};
};

static CTLSMemoryPool<SendPacketInfo>* sendPacketInfoPool = new CTLSMemoryPool<SendPacketInfo>(4, false);