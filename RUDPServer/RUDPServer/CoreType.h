#pragma once

using SessionId = unsigned long long;
const SessionId invalidSessionId = -1;

using PacketSequence = unsigned long long;

using PacketRetransmissionCount = unsigned short;
const PacketRetransmissionCount maxPacketRetransmissionCount = 8;

const unsigned long long RetransmissionCheckTime = 8;

#define USE_RETRANSMISSION_SLEEP 1
#define OUT

enum class PACKET_ID : unsigned int
{
	InvalidPacketId = 0
	, Ping
	, Pong
};