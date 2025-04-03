#include <Windows.h>
#include "Main.h"
#include "MetaHook.h"
#include "Utils.h"

unsigned long m_iHostIPAddress;
unsigned short m_iHostPort;

void WriteBytes(PVOID address, void* val, int bytes) 
{
	DWORD d, ds;
	VirtualProtect(address, bytes, PAGE_EXECUTE_READWRITE, &d);
	memcpy(address, val, bytes);
	VirtualProtect(address, bytes, d, &ds);
}

namespace HackShield
{
	void(__cdecl* oSetACInit)();
	void SetACInit()
	{

	}

	void(__cdecl* oHook1)();
	void Hook1()
	{

	}

	bool(__cdecl* oHook2)();
	bool Hook2()
	{
		oHook2();
		return 1;
	}
}

namespace CSONMWrapper
{
	std::string m_sLastSubmitAccount;
	std::string m_sLastSubmitPassword;
	char(__thiscall* oCSONMWrapper__AuthUser)(void* thisptr, const char* szAccount, const char* szPassword, int a4);
	char __fastcall AuthUser(void* thisptr, void* edx, const char* szAccount, const char* szPassword, int a4)
	{
		m_sLastSubmitAccount = szAccount;
		m_sLastSubmitPassword = szPassword;
		return 1;
	}

	bool(__cdecl* oCheckIsAge18)();
	bool __cdecl CheckIsAge18()
	{
		return 1;
	}

	void(__stdcall* oCOutPacket__SendLoginPacket)(DWORD** account, DWORD** password);
	void __stdcall COutPacket__SendLoginPacket(DWORD** account, DWORD** password)
	{
		//NMCO Bypass後的問題 會導致登入包的帳密被破壞
		DWORD dwAccountTable = (DWORD)*account;
		DWORD dwPasswordTable = (DWORD)*password;

		*(DWORD*)(dwAccountTable - 0xC) = m_sLastSubmitAccount.length();
		memcpy((void*)dwAccountTable, m_sLastSubmitAccount.c_str(), m_sLastSubmitAccount.length());
		memcpy((void*)(dwAccountTable + m_sLastSubmitAccount.length()), "\0", 1);

		*(DWORD*)(dwPasswordTable - 0xC) = m_sLastSubmitPassword.length();
		memcpy((void*)dwPasswordTable, m_sLastSubmitPassword.c_str(), m_sLastSubmitPassword.length());
		memcpy((void*)(dwPasswordTable + m_sLastSubmitPassword.length()), "\0", 1);
		oCOutPacket__SendLoginPacket(account, password);

		m_sLastSubmitAccount.clear();
		m_sLastSubmitPassword.clear();
	}
}

namespace HookFuncs
{
	char m_iEnableSSL = 1;
	int(__cdecl* oSocketManager__Constructor)(char* a1, char a2);
	int __cdecl SocketManager__Constructor(char* a1, char a2)
	{
		return oSocketManager__Constructor(a1, m_iEnableSSL);
	}

	int(__thiscall* oIpRedirector)(void* pThis, int ip, __int16 port, char a4);
	int __fastcall IpRedirector(void* pThis, void* edx, int ip, __int16 port, char a4)
	{
		return oIpRedirector(pThis, m_iHostIPAddress, m_iHostPort, a4);
	}

	int(__cdecl* oHolePunchFuncSetServerInfo)(int ip, __int16 port);
	int __cdecl HolePunchFuncSetServerInfo(int ip, __int16 port)
	{
		return oHolePunchFuncSetServerInfo(m_iHostIPAddress, m_iHostPort);
	}
}

void GamePatcher() 
{
	Utils::AttachConsole();
	
	std::string sIpAddr = CommandLine()->GetParmValue("-ip");
	unsigned short nPort = CommandLine()->GetParmValue("-port", 0);
	m_iHostIPAddress = inet_addr(sIpAddr.c_str());
	m_iHostPort = htons(nPort);
	if (CommandLine()->CheckParm("-nossl") != NULL)
		HookFuncs::m_iEnableSSL = 0;

	Utils::ConsolePrint("IP: %s\n", sIpAddr.c_str());
	Utils::ConsolePrint("Port: %d\n", nPort);

	MH_InlineHook((void*)0x37297D80, HackShield::SetACInit, (void*&)HackShield::oSetACInit);
	MH_InlineHook((void*)0x37442F60, HackShield::Hook2, (void*&)HackShield::oHook2);
	WriteBytes((void*)0x37297AE2, (void*)"\xEB\x5D", 2); //HackShield

	MH_InlineHook((void*)0x374405F0, HookFuncs::SocketManager__Constructor, (void*&)HookFuncs::oSocketManager__Constructor);
	MH_InlineHook((void*)0x37444120, HookFuncs::IpRedirector, (void*&)HookFuncs::oIpRedirector);
	MH_InlineHook((void*)0x371F4470, HookFuncs::HolePunchFuncSetServerInfo, (void*&)HookFuncs::oHolePunchFuncSetServerInfo);

	MH_InlineHook((void*)0x37442A60, CSONMWrapper::AuthUser, (void*&)CSONMWrapper::oCSONMWrapper__AuthUser);
	MH_InlineHook((void*)0x37442810, CSONMWrapper::CheckIsAge18, (void*&)CSONMWrapper::oCheckIsAge18);
	MH_InlineHook((void*)0x372FBDB0, CSONMWrapper::COutPacket__SendLoginPacket, (void*&)CSONMWrapper::oCOutPacket__SendLoginPacket);
	WriteBytes((void*)0x372207B0, (void*)"\x31\xC0", 2);


}