#pragma once
#include "PacketReader.h"

class IPacketHandler
{
public:
	virtual ~IPacketHandler();
	virtual int GetOpcode() const = 0;
	virtual void OnReceive(void* pBuffer, int nLength) = 0;
};

class CPacketManager
{
public:
	void Initialize();
	bool RegisterPacket(IPacketHandler* pHandler);
private:
	bool m_bInitialized = false;
}; extern CPacketManager g_PacketManager;
