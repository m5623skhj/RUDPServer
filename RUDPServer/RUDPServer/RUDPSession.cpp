#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"
#include "CoreUtil.h"
#include <ranges>
#include "Protocol.h"
#include "PacketManager.h"
#include "RUDPServerCore.h"

RUDPSession::RUDPSession(RUDPServerCore& inServerCore, const SessionId inSessionId)
	: serverCore(inServerCore)
	, sessionId(inSessionId)
{
	clientAddr = RUDPCoreUtil::MakeAddressInfoFromSessionId(sessionId);
}

void RUDPSession::SendPacket(IPacket& packet, const SessionId targetSessionId)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "Buffer is nullptr in RUDPSession::SendPacket()" << std::endl;
		return;
	}

	*buffer << packet.GetPacketId();
	packet.PacketToBuffer(*buffer);

	auto sendPacketInfo = sendPacketInfoPool->Alloc();
	if (sendPacketInfo == nullptr)
	{
		std::cout << "SendPacketInfo is nullptr in RUDPSession::SendPacket()" << std::endl;
		return;
	}

	sendPacketInfo->Initialize(sessionId, targetSessionId, buffer);
	serverCore.SendPacketTo(sendPacketInfo);
}

SessionId RUDPSession::GetSessionId()
{
	return sessionId;
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

void RUDPSession::OnConnected()
{
	if (auto packetHandler = PacketManager::GetInst().GetPacketHandler(static_cast<PacketId>(PACKET_ID::Connect)))
	{
		Connect packet;
		PacketManager::HandlePacket(*this, packet);
	}
}

void RUDPSession::OnSessionReleased()
{
	if (auto packetHandler = PacketManager::GetInst().GetPacketHandler(static_cast<PacketId>(PACKET_ID::Disconnect)))
	{
		Disconnect packet;
		PacketManager::HandlePacket(*this, packet);
	}
}