#pragma once

using SessionId = unsigned long long;
const SessionId invalidSessionId = -1;

using PacketSequence = unsigned long long;

using PacketRetransmissionCount = unsigned short;
const PacketRetransmissionCount maxPacketRetransmissionCount = 8;