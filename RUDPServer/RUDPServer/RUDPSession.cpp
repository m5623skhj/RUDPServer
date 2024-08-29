#pragma once
#include "PreCompile.h"
#include "RUDPSession.h"

SendPacketSequeceManager::SendPacketSequeceManager()
{
	Initialize();
}

void SendPacketSequeceManager::Initialize()
{
	for (auto& iter : sequenceManager)
	{
		for (auto& innerIter : iter.second)
		{
			NetBuffer::Free(innerIter.second.buffer);
		}

		iter.second.clear();
	}

	sequenceManager.clear();
}

RUDPSession::RUDPSession(SessionId inSessionId)
	: sessionId(inSessionId)
{
	sendPacketSequenceManager.Initialize();
}

void RUDPSession::OnTick()
{
	CheckAndRetransmitPacket();
}

void RUDPSession::CheckAndRetransmitPacket()
{

}

bool RUDPSession::CheckMaxRetransmitCount(PacketRetransmissionCount retransmissionCount)
{
	return maxPacketRetransmissionCount <= retransmissionCount;
}