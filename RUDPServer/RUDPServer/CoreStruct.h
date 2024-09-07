#pragma once
#include "CoreType.h"
#include <chrono>
#include "NetServerSerializeBuffer.h"

struct SendPacketInfo
{
	bool isSendedPacket{};
	SessionId sendOwner{};
	std::chrono::time_point<std::chrono::steady_clock> timestamp{};
	PacketRetransmissionCount retransmissionCount{};
	NetBuffer* buffer{};
};
