#pragma once
#include "PacketManager.h"

class CFriendPacket : public IPacketHandler
{
public:
	int GetOpcode() const override;
	void OnReceive(void* pBuffer, int nLength) override;
}; extern CFriendPacket g_FriendPacket;
