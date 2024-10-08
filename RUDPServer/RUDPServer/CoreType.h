#pragma once

using SessionId = unsigned long long;
const SessionId invalidSessionId = -1;

using PlayerId = unsigned long long;
const PlayerId invalidPlayerId = -1;

using PacketSequence = unsigned long long;

using PacketRetransmissionCount = unsigned short;
const PacketRetransmissionCount maxPacketRetransmissionCount = 8;

const unsigned long long retransmissionCheckTime = 8;
const unsigned long long deleteThreadCheckTime = 1000;

#define USE_RETRANSMISSION_SLEEP 1
#define OUT

enum class PACKET_ID : unsigned int
{
	InvalidPacketId = 0
	, Ping
	, Pong
	, Connect
	, Disconnect
};

enum class PACKET_TYPE : unsigned char
{
	InvalidType = 0
	, ConnectType
	, DisconnectType
	, SendType
	, SendReplyType
};