#pragma once
#include "CoreType.h"
#include <chrono>
#include "NetServerSerializeBuffer.h"

struct SendPacketInfo
{
public:
	SendPacketInfo()
		: timestamp(std::chrono::steady_clock::now())
	{

	}
	~SendPacketInfo() = default;

	void Initialize(const SessionId inSendOwnerId, const SessionId inSendTargetId, NetBuffer* inPacket)
	{
		sendOwner = inSendOwnerId;
		sendTargetId = inSendTargetId;
		buffer = inPacket;
		NetBuffer::AddRefCount(inPacket);
	}

public:
	void FreeBuffer()
	{
		NetBuffer::Free(buffer);
		buffer = nullptr;
	}

	[[nodiscard]]
	SessionId GetSendOwner() const { return sendOwner; }
	[[nodiscard]]
	SessionId GetSendTarget() const { return sendTargetId; }
	[[nodiscard]]
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