// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#define VERSION "0.1.2"
#define TARGETVERSION "4.3.3"

#ifdef _WIN64
typedef unsigned long long ADDR_LEN;
#else
typedef unsigned long ADDR_LEN;
#endif

template<size_t iDataSize>
static void WriteProtectedMemory(uintptr_t pDestination, const std::array<uint8_t, iDataSize>& data)
{
	DWORD dwOldProtect;
	if (VirtualProtect((LPVOID)pDestination, iDataSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		std::copy(data.begin(), data.end(), reinterpret_cast<uint8_t*>(pDestination));
		VirtualProtect((LPVOID)pDestination, iDataSize, dwOldProtect, NULL);
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)pDestination, iDataSize);
	}
}

void Sys_RunInit()
{
	ADDR_LEN hModule = (ADDR_LEN)GetModuleHandle(NULL);
#ifndef _WIN64
	if (*(DWORD*)(hModule + 0x742D8) == 0x8514C483) //SM4.2.3 !
	{
		*(BYTE*)(hModule + 0x742DD) = 0x75;
		if (_access("_edu_", 0) != -1)
			*(BYTE*)(hModule + 0x74319) = 0x75;
		else
			*(BYTE*)(hModule + 0x742F2) = 0x75;
	}
	else
	{
		MessageBox(NULL, "Your ShaderMap version is incompatible with SM4Hook " VERSION "\r\nThis is made for " TARGETVERSION, "SM4Hook " VERSION, MB_ICONERROR);
		ExitProcess(-1);
	}
#else
	if (*(BYTE*)(hModule + 0x885D0) == 0x0F) //SM4.3.3 x64!
	{
		const std::array<uint8_t, 4> patch = { 0xE9, 0x4D, 0x08, 0x00 };
		const std::array<uint8_t, 1> patch1 = { 0x01 };
		WriteProtectedMemory((uintptr_t)(hModule + 0x885D0), patch);
		WriteProtectedMemory((uintptr_t)(hModule + 0x88E2F), patch1);
	}
	else
	{
		MessageBox(NULL, "Your ShaderMap version is incompatible with SM4Hook " VERSION "\r\nThis is made for " TARGETVERSION, "SM4Hook " VERSION, MB_ICONERROR);
		ExitProcess(-1);
	}
#endif
}

bool __stdcall DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		Sys_RunInit();
	}

	return true;
}