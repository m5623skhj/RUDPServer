#pragma once

#include <string>
#include <functional>
#include "CoreType.h"
#include "EnumType.h"
#include "NetServerSerializeBuffer.h"

using PacketId = unsigned int;

#define GET_PACKET_ID(packetId) virtual PacketId GetPacketId() const override { return static_cast<PacketId>(packetId); }

#pragma region ForGameServerPacket
template<typename T>
void SetBufferToParameters(OUT NetBuffer& buffer, T& param)
{
	buffer >> param;
}

template<typename T, typename... Args>
void SetBufferToParameters(OUT NetBuffer& buffer, T& param, Args&... argList)
{
	buffer >> param;
	SetBufferToParameters(buffer, argList...);
}

template<typename T>
void SetParametersToBuffer(OUT NetBuffer& buffer, T& param)
{
	buffer << param;
}

template<typename T, typename... Args>
void SetParametersToBuffer(OUT NetBuffer& buffer, T& param, Args&... argList)
{
	buffer << param;
	SetParametersToBuffer(buffer, argList...);
}

#define SET_BUFFER_TO_PARAMETERS(...)\
virtual void BufferToPacket(OUT NetBuffer& buffer) override { SetBufferToParameters(buffer, __VA_ARGS__); }

#define SET_PARAMETERS_TO_BUFFER(...)\
virtual void PacketToBuffer(OUT NetBuffer& buffer) override { SetParametersToBuffer(buffer, __VA_ARGS__); }

// This function assembles the packet based on the order of the defined parameters
#define SET_PARAMETERS(...)\
	SET_BUFFER_TO_PARAMETERS(__VA_ARGS__)\
	SET_PARAMETERS_TO_BUFFER(__VA_ARGS__)

#pragma endregion ForGameServerPacket

////////////////////////////////////////////////////////////////////////////////////
// Packet
////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;

public:
	virtual void BufferToPacket(OUT NetBuffer& buffer) { UNREFERENCED_PARAMETER(buffer); }
	virtual void PacketToBuffer(OUT NetBuffer& buffer) { UNREFERENCED_PARAMETER(buffer); }
};

class Ping : public IPacket
{
public:
	Ping() = default;
	virtual ~Ping() override = default;

public:
	GET_PACKET_ID(PACKET_ID::Ping)
};

class Pong : public IPacket
{
public:
	Pong() = default;
	virtual ~Pong() override = default;

public:
	GET_PACKET_ID(PACKET_ID::Pong);
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////
// Packet Register
////////////////////////////////////////////////////////////////////////////////////

#pragma region PacketHandler
#define REGISTER_PACKET(PacketType){\
	RegisterPacket<PacketType>();\
	RegisterPacketHandler<PacketType>();\
}

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(RUDPSession& session, PacketType& packet);\

#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(Ping)\

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(Ping)\
}

#pragma endregion PacketHandler