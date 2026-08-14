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

bool SetIMEEnabled(HWND hwnd_, bool enabled)
{
	HIMC himc = ImmGetContext(hwnd_);
	if (!himc)
		return false;
	BOOL current = ImmGetOpenStatus(himc);
	if (current == (enabled ? TRUE : FALSE))
	{
		ImmReleaseContext(hwnd_, himc);
		return true;
	}
	BOOL ok = ImmSetOpenStatus(himc, enabled ? TRUE : FALSE);
	ImmReleaseContext(hwnd_, himc);
	return ok == TRUE;
}

namespace RankLocationFix
{
	unsigned int(__thiscall* oRankSetPlayerInfo)(void* thisptr, int a2);
	void RefreshDisplay(void* thisptr)
	{
		// (sub_372FFB10 > LABEL_27)
		DWORD pBegin = *(DWORD*)((char*)thisptr + 0x410);
		DWORD pEnd = *(DWORD*)((char*)thisptr + 0x414);
		for (DWORD ptr = pBegin; ptr < pEnd; ptr += 4)
		{
			DWORD observer = *(DWORD*)ptr;
			if (observer)
			{
				DWORD vtable = *(DWORD*)observer;
				reinterpret_cast<void(__thiscall*)(void*)>(*(DWORD*)(vtable + 0x10))((void*)observer);
			}
		}
	}

	unsigned int __fastcall RankSetPlayerInfo(void* thisptr, void* edx, int a2)
	{
		int regionType = *(int*)((char*)a2 + 0x20);

		// 顯示地區為1的時候不處理，1為只顯示城市
		if (regionType != 2 && regionType != 3)
			return oRankSetPlayerInfo(thisptr, a2);

		unsigned int packed = *(unsigned int*)((char*)a2 + 0x24);
		// cstrike_tw/resource/location.csv
		unsigned int city = 0, district = 0, village = 0;

		if (regionType == 2)
		{
			// 端口發包格式：(縣市 << 10) | 鄉鎮區
			district = packed & 0x3FF;
			city = (packed >> 10) & 0xFF;
		}
		else
		{
			// 端口發包格式：(縣市 << 24) | (鄉鎮區 << 14) | 村里
			village = packed & 0x3FFF;
			district = (packed >> 14) & 0x3FF;
			city = (packed >> 24) & 0xFF;
		}

		*(int*)((char*)a2 + 0x20) = 1; // 將類別改為1，避免觸發壞掉的type 2/3
		*(unsigned int*)((char*)a2 + 0x24) = city;

		// 暫時清空vector，讓原函式內部的UI刷新迴圈跑空 (sub_372FFB10 > LABEL_27)
		DWORD* pBegin = (DWORD*)((char*)thisptr + 0x410);
		DWORD* pEnd = (DWORD*)((char*)thisptr + 0x414);
		DWORD savedEnd = *pEnd;
		*pEnd = *pBegin;

		auto ret = oRankSetPlayerInfo(thisptr, a2);

		// 還原vector
		*pEnd = savedEnd;

		// 補上鄉鎮區與村里 Index
		*(DWORD*)((char*)thisptr + 0x3D8) = district;
		*(DWORD*)((char*)thisptr + 0x3DC) = village;

		RefreshDisplay(thisptr);
		return ret;
	}
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

	static bool m_bWasChatting = false;
	static bool m_bIMEDisabled = false;
	static bool m_bWasInGame = false;
	LRESULT(__thiscall* oCGame__WndProc)(void* thisptr, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT __fastcall CGame__WndProc(void* thisptr, void* edx, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		bool bCursorShowAndInChatInput = *(bool*)(0x375B6E81 + 0x01);
		bool bInGame = *(int*)(0x38F7F8A0) >= 5;

		if (bInGame && !bCursorShowAndInChatInput)
		{
			switch (Msg)
			{
			case WM_IME_STARTCOMPOSITION:
			case WM_IME_ENDCOMPOSITION:
			case WM_IME_COMPOSITION:
			case WM_IME_SETCONTEXT:
				return 0;
			}
		}

		LRESULT ret = oCGame__WndProc(thisptr, hWnd, Msg, wParam, lParam);

		if (Msg == WM_ACTIVATEAPP && !wParam)
			m_bIMEDisabled = false;

		if (bInGame && !m_bWasInGame)
			m_bIMEDisabled = false;

		m_bWasInGame = bInGame;

		if (bCursorShowAndInChatInput && !m_bWasChatting)
		{
			SetIMEEnabled(hWnd, true);
			m_bIMEDisabled = false;
		}

		if (!bCursorShowAndInChatInput && m_bWasChatting && bInGame)
		{
			SetIMEEnabled(hWnd, false);
			m_bIMEDisabled = true;
		}

		m_bWasChatting = bCursorShowAndInChatInput;

		if (bInGame && !bCursorShowAndInChatInput)
		{
			switch (Msg)
			{
			case WM_ACTIVATEAPP:
				if (wParam)
				{
					SetIMEEnabled(hWnd, false);
					m_bIMEDisabled = true;
				}
				break;
			case WM_INPUTLANGCHANGE:
				SetIMEEnabled(hWnd, false);
				m_bIMEDisabled = true;
				break;
			case WM_IME_NOTIFY:
				if (wParam == IMN_SETOPENSTATUS)
				{
					SetIMEEnabled(hWnd, false);
					m_bIMEDisabled = true;
				}
				break;
			}
		}

		return ret;
	}

	ULONGLONG m_ullLastSendOptionPacketTime = GetTickCount64();
	typedef int(__thiscall* COptionsDialog__OnCommand_t)(void* thisptr, char* szButtonName); // aka PropertyDialog::OnCommand
	COptionsDialog__OnCommand_t oCOptionsDialog__OnCommand = nullptr;
	int __fastcall COptionsDialog__OnCommand(void* thisptr, void* edx, char* szButtonName)
	{
		// 56 57 8B 7C 24 ? 68 ? ? ? ? 8B F1 57 E8 ? ? ? ? 83 C4 ? 85 C0 75 ? 8B 06 8B CE FF 90 ? ? ? ? 8B 16
		auto ret = oCOptionsDialog__OnCommand(thisptr, szButtonName);
		auto ullNow = GetTickCount64();
		if ((strcmp(szButtonName, "Apply") == 0 || strcmp(szButtonName, "OK") == 0) && ullNow - m_ullLastSendOptionPacketTime > 300)
		{
			reinterpret_cast<void(__cdecl*)()>(0x371786D0)();
			m_ullLastSendOptionPacketTime = ullNow;
		}
		return ret;
	}
}

DWORD WINAPI SubModulePatcher()
{
	uintptr_t dwGameUI = NULL;
	uintptr_t dwClient = NULL;
	while (true)
	{
		dwGameUI = (uintptr_t)GetModuleHandleA("GameUI.dll");
		dwClient = (uintptr_t)GetModuleHandleA("client.dll");
		if (dwGameUI && dwClient)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	// Waiting for memory initialization
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	HookFuncs::oCOptionsDialog__OnCommand = reinterpret_cast<HookFuncs::COptionsDialog__OnCommand_t>(dwGameUI + 0x1E3220);
	// Vtable hook
	MH_WriteDWORD((void*)(dwGameUI + 0x2670CC), (DWORD)&HookFuncs::COptionsDialog__OnCommand);
	return 1;
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
	WriteBytes((void*)0x37298B40, (void*)"\xC3", 1); //HackShield

	MH_InlineHook((void*)0x374405F0, HookFuncs::SocketManager__Constructor, (void*&)HookFuncs::oSocketManager__Constructor);
	MH_InlineHook((void*)0x37444120, HookFuncs::IpRedirector, (void*&)HookFuncs::oIpRedirector);
	MH_InlineHook((void*)0x371F4470, HookFuncs::HolePunchFuncSetServerInfo, (void*&)HookFuncs::oHolePunchFuncSetServerInfo);

	MH_InlineHook((void*)0x372FFB10, RankLocationFix::RankSetPlayerInfo, (void*&)RankLocationFix::oRankSetPlayerInfo);
	MH_InlineHook((void*)0x371EDDF0, HookFuncs::CGame__WndProc, (void*&)HookFuncs::oCGame__WndProc);

	MH_InlineHook((void*)0x37442A60, CSONMWrapper::AuthUser, (void*&)CSONMWrapper::oCSONMWrapper__AuthUser);
	MH_InlineHook((void*)0x37442810, CSONMWrapper::CheckIsAge18, (void*&)CSONMWrapper::oCheckIsAge18);
	MH_InlineHook((void*)0x372FBDB0, CSONMWrapper::COutPacket__SendLoginPacket, (void*&)CSONMWrapper::oCOutPacket__SendLoginPacket);
	WriteBytes((void*)0x372207B0, (void*)"\x31\xC0", 2);

	CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)SubModulePatcher, nullptr, 0, nullptr);
}