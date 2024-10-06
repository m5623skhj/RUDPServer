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
	replyWaitingMap.clear();
}

void RUDPSession::SendPacket(IPacket& packet, const SessionId targetSessionId)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "Buffer is nullptr in RUDPSession::SendPacket()" << std::endl;
		return;
	}

	auto sendPacketInfo = sendPacketInfoPool->Alloc();
	if (sendPacketInfo == nullptr)
	{
		std::cout << "SendPacketInfo is nullptr in RUDPSession::SendPacket()" << std::endl;
		NetBuffer::Free(buffer);
		return;
	}

	PACKET_TYPE packetType = PACKET_TYPE::SendType;
	unsigned long long packetSequence = ++lastSendPacketSequence;
	*buffer << packetType << packetSequence << packet.GetPacketId();
	packet.PacketToBuffer(*buffer);

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

bool RUDPSession::OnConnected(NetBuffer& recvPacket)
{
	PacketId packetId;
	recvPacket >> packetId;

	if (packetId != static_cast<PacketId>(PACKET_ID::Connect))
	{
		return false;
	}

	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return false;
	}

	if (auto packetHandler = PacketManager::GetInst().GetPacketHandler(packetId))
	{
		packet->BufferToPacket(recvPacket);
		return PacketManager::HandlePacket(*this, *(static_cast<Connect*>(packet.get())));
	}

	return false;
}

void RUDPSession::OnSessionReleased()
{
	if (auto packetHandler = PacketManager::GetInst().GetPacketHandler(static_cast<PacketId>(PACKET_ID::Disconnect)))
	{
		Disconnect packet;
		PacketManager::HandlePacket(*this, packet);
	}
}

SendPacketInfo* RUDPSession::GetSendPacketInfo(PacketSequence recvPacketSequence)
{
	SendPacketInfo* waitingReplyInfo = nullptr;
	{
		std::shared_lock lock(replyWaitingMapLock);

		auto itor = replyWaitingMap.find(recvPacketSequence);
		if (itor != replyWaitingMap.end())
		{
			waitingReplyInfo = itor->second;
		}
	}

	return waitingReplyInfo;
}

bool RUDPSession::DeleteSendPacketInfo(PacketSequence recvPacketSequence)
{
	SendPacketInfo* waitingReplyInfo = nullptr;
	{
		std::shared_lock lock(replyWaitingMapLock);

		auto itor = replyWaitingMap.find(recvPacketSequence);
		if (itor == replyWaitingMap.end())
		{
			return false;
		}

		waitingReplyInfo = itor->second;
	}

	{
		std::unique_lock lock(replyWaitingMapLock);
	
		replyWaitingMap.erase(recvPacketSequence);
	}

	if (waitingReplyInfo != nullptr)
	{
		sendPacketInfoPool->Free(waitingReplyInfo);
	}

	return true;
}