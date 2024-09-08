#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"
#include "CoreUtil.h"
#include <ranges>

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
	CheckAndRetransmissionPacket();
}

void RUDPSession::CheckAndRetransmissionPacket()
{

}

bool RUDPSession::CheckMaxRetransmissionCount(PacketRetransmissionCount retransmissionCount)
{
	return maxPacketRetransmissionCount <= retransmissionCount;
}

void RUDPSession::OnSessionReleased()
{

}