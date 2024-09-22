#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"
#include "CoreUtil.h"
#include <ranges>
#include "Protocol.h"
#include "PacketManager.h"
#include "RUDPServerCore.h"

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

RUDPSession::RUDPSession(RUDPServerCore& inServerCore, const SessionId inSessionId)
	: serverCore(inServerCore)
	, sessionId(inSessionId)
{
	sendPacketSequenceManager.Initialize();
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

void RUDPSession::OnSessionReleased()
{

}