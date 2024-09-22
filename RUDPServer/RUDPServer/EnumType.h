#pragma once

enum class PacketId : unsigned int
{
	InvalidPacketId = 0
	, Ping
	, Pong
};