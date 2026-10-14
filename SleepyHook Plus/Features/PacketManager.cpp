#include "..//Main.h"
#include "..//Utils.h"
#include "..//MetaHook.h"
#include "PacketManager.h"
#include "PacketFriend.h"

CPacketManager g_PacketManager;

namespace PacketRegistry
{
	typedef void* (__thiscall* Packet_Ctor_t)(void* thisptr);
	typedef void* (__thiscall* Packet_Dtor_t)(void* thisptr);
	typedef void* (__cdecl* ClientNew_t)(size_t nSize); // operator new
	typedef void (__cdecl* ClientDelete_t)(void* pBlock); // operator delete

	Packet_Ctor_t Packet_Ctor = reinterpret_cast<Packet_Ctor_t>(0x3743E030);
	Packet_Dtor_t Packet_Dtor = reinterpret_cast<Packet_Dtor_t>(0x3743E070);
	ClientNew_t ClientNew = reinterpret_cast<ClientNew_t>(0x3741B110);
	ClientDelete_t ClientDelete = reinterpret_cast<ClientDelete_t>(0x3741AE40);
	void* s_pVFTable[3] = { nullptr, nullptr, nullptr };
	struct PacketObject
	{
		void* pVFTable;
		BYTE nBaseFlag;
		BYTE pad[3];
		DWORD Unk08;
		DWORD dwListenerHead;
		DWORD Unk10;
		DWORD Unk14;
		IPacketHandler* pOwner;
		int nOpcode;
	};
	static_assert(sizeof(PacketObject) == 0x20);

	DWORD* GetSlot(int nOpcode)
	{
		if (nOpcode < 0 || nOpcode >= 256)
			return nullptr;
		DWORD dwSocketManager = *(DWORD*)(0x383F867C);
		if (!dwSocketManager)
			return nullptr;
		return (DWORD*)(dwSocketManager + 0x10 + 4 * nOpcode);
	}

	void* __fastcall Packet_ScalarDeletingDtor(PacketObject* thisptr, void* edx, char iFlag)
	{
		if (!thisptr)
			return thisptr;
		DWORD* pSlot = GetSlot(thisptr->nOpcode);
		if (pSlot && *pSlot == (DWORD)thisptr)
			*pSlot = 0;
		thisptr->pOwner = nullptr;
		Packet_Dtor(thisptr);
		if (iFlag & 1)
			ClientDelete(thisptr);
		return thisptr;
	}

	char __fastcall Packet_Receive(PacketObject* thisptr, void* edx, void* pBuffer, int nLength)
	{
		if (thisptr && thisptr->pOwner && pBuffer && nLength > 0)
			thisptr->pOwner->OnReceive(pBuffer, nLength);
		return 1;
	}

	void __fastcall Packet_Reset(PacketObject* thisptr, void* edx)
	{
	}

	int(__cdecl* oRegisterInPacketHandlers)();
	int __cdecl RegisterInPacketHandlers()
	{
		int iResult = oRegisterInPacketHandlers();
		g_PacketManager.RegisterPacket(&g_FriendPacket);
		return iResult;
	}
}

namespace PacketMgrHookFuncs
{
	int(__thiscall* oCSocket__ReadPacket)(void* thisptr, char* buffer, int maxBufferSize, int* bufferLen, BOOL bWaitHandshake);
	int __fastcall CSocket__ReadPacket_hook(void* thisptr, void* edx, char* outBuffer, int maxBufferSize, int* outBufferLength, BOOL bWaitHandshake)
	{
		int errorCode = oCSocket__ReadPacket(thisptr, outBuffer, maxBufferSize, outBufferLength, bWaitHandshake);
		if (bWaitHandshake || errorCode || !outBuffer)
			return errorCode;
		CPacketReader pkt(outBuffer, *outBufferLength);
		int packetId = pkt.Read<unsigned char>();

		// 你也可以透過這裡解析你的自訂封包，不透過 RegisterPacket 註冊封包 (You can also parse your custom packets here without using RegisterPacket)


		/*
		unsigned char* buffer = reinterpret_cast<unsigned char*>(outBuffer);
		Utils::ConsolePrint("[PacketID: %02d] Data: ", packetId);
		for (int i = 0; i < *outBufferLength; i++)
		{
			Utils::ConsolePrint("%02X ", buffer[i]);
		}
		Utils::ConsolePrint("\n");
		*/

		return errorCode;
	}
}

IPacketHandler::~IPacketHandler()
{

}

bool CPacketManager::RegisterPacket(IPacketHandler* pHandler)
{
	if (!pHandler)
		return false;

	int nOpcode = pHandler->GetOpcode();
	DWORD* pSlot = PacketRegistry::GetSlot(nOpcode);
	if (!pSlot)
		return false;

	void* pBlock = PacketRegistry::ClientNew(sizeof(PacketRegistry::PacketObject));
	if (!pBlock)
		return false;

	PacketRegistry::Packet_Ctor(pBlock);

	PacketRegistry::PacketObject* pObject = (PacketRegistry::PacketObject*)pBlock;
	pObject->pVFTable = PacketRegistry::s_pVFTable;
	pObject->nBaseFlag = 0;
	pObject->pOwner = pHandler;
	pObject->nOpcode = nOpcode;

	*pSlot = (DWORD)pObject;
	return true;
}

void CPacketManager::Initialize()
{
	if (m_bInitialized)
		return;

	uintptr_t dwCSocket__ReadPacket = Utils::FindSignature("hw.dll", "64 A1 ? ? ? ? 6A ? 68 ? ? ? ? 50 8A 54 24");
	if (!dwCSocket__ReadPacket)
	{
		MessageBoxW(NULL, L"Cannot find CSocket::ReadPacket", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	MH_InlineHook(dwCSocket__ReadPacket, PacketMgrHookFuncs::CSocket__ReadPacket_hook, (void*&)PacketMgrHookFuncs::oCSocket__ReadPacket);

	PacketRegistry::s_pVFTable[0] = reinterpret_cast<void*>(&PacketRegistry::Packet_ScalarDeletingDtor);
	PacketRegistry::s_pVFTable[1] = reinterpret_cast<void*>(&PacketRegistry::Packet_Receive);
	PacketRegistry::s_pVFTable[2] = reinterpret_cast<void*>(&PacketRegistry::Packet_Reset);
	MH_InlineHook(0x37293B20, PacketRegistry::RegisterInPacketHandlers, (void*&)PacketRegistry::oRegisterInPacketHandlers);

	m_bInitialized = true;
}