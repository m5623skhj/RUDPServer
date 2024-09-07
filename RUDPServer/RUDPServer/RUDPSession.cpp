#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"
#include "CoreUtil.h"

SendPacketSequeceManager::SendPacketSequeceManager()
{
	Initialize();
}

void SendPacketSequeceManager::Initialize()
{
	for (auto iter : sequenceManager)
	{
		NetBuffer::Free(iter.second.buffer);
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