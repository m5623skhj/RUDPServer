#pragma once
#include <unordered_map>
#include "CoreStruct.h"

class IPacket;

class RUDPServerCore;

class SendPacketSequeceManager final
{
public:
	SendPacketSequeceManager();
	~SendPacketSequeceManager() = default;

public:
	void Initialize();

private:
	// ���� �ʿ����� ���Ŀ� ���� �ʿ�
	std::unordered_map<PacketSequence, SendPacketInfo> sequenceManager;
};

class RUDPSession
{
	friend RUDPServerCore;

public:
	RUDPSession() = delete;
	explicit RUDPSession(RUDPServerCore& inServerCore, const SessionId inSessionId);
	~RUDPSession() = default;

public:
	void SendPacket(IPacket& packet, const SessionId targetSessionId);
	SessionId GetSessionId();

private:
	void OnRecvPacket(NetBuffer& recvPacket);
	void OnSessionReleased();

private:
	PacketSequence lastSendPacketSequence{};

private:
	SendPacketSequeceManager sendPacketSequenceManager;

private:
	SessionId sessionId{};
	SOCKADDR_IN clientAddr{};
	RUDPServerCore& serverCore;
};