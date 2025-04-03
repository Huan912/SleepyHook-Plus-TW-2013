#pragma once
#include <Windows.h>
#include <string>
#include <vector>
namespace Utils
{
	void AttachConsole();
	void DetachConsole();
	bool ConsolePrint(const char* fmt, ...);
	std::vector<char> HexToBytes(const std::string& hex);
	uintptr_t FindSignature(const char* szModule, const char* szSignature);
}