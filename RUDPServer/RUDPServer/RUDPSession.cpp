#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"
#include "CoreUtil.h"
#include <ranges>
#include "Protocol.h"
#include "PacketManager.h"

SendPacketSequeceManager::SendPacketSequeceManager()
{
	Initialize();
}

void SendPacketSequeceManager::Initialize()
{
	for (auto& value : sequenceManager | std::views::values)
	{
		value.FreeBuffer();
	}

	sequenceManager.clear();
}

RUDPSession::RUDPSession(SessionId inSessionId)
{
	sendPacketSequenceManager.Initialize();
	sessionId = inSessionId;
	clientAddr = RUDPCoreUtil::MakeAddressInfoFromSessionId(sessionId);
}

void RUDPSession::OnTick()
{

}

void RUDPSession::OnRecvPacket(NetBuffer& recvPacket)
{
	PacketId packetId;
	recvPacket >> packetId;

	auto packetHandler = PacketManager::GetInst().GetPacketHandler(packetId);
	if (packetHandler == nullptr)
	{
		return;
	}

	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return;
	}

	char* targetPtr = reinterpret_cast<char*>(packet.get()) + sizeof(char*);
	std::any anyPacket = std::any(packet.get());
	packetHandler(*this, recvPacket, anyPacket);
}

void RUDPSession::OnSessionReleased()
{

}