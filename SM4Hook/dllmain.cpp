// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#define VERSION "0.1.0"
#define TARGETVERSION "4.2.1"

#ifdef _WIN64
typedef unsigned long long ADDR_LEN;

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

#else
typedef unsigned long ADDR_LEN;
#endif

void Sys_RunInit()
{
	ADDR_LEN hModule = (ADDR_LEN)GetModuleHandle(NULL);
#ifndef _WIN64
	if (*(WORD*)(hModule + 0x1000) == 0x5068) //SM4.2.1 !
	{
		*(BYTE*)(hModule + 0x741AF) = 0x75;
		if (_access("_edu_", 0) != -1)
			*(BYTE*)(hModule + 0x741EB) = 0x75;
		else
			*(BYTE*)(hModule + 0x741C4) = 0x75;

		*(ADDR_LEN*)(hModule + 0x79CF6) = (ADDR_LEN)L"ShaderMap™ - SM4Hook " VERSION " Installed";
		*(ADDR_LEN*)(hModule + 0x74639) = (ADDR_LEN)L"SM4HOOK LICENSE";
	}
	else
	{
		MessageBox(NULL, "Your ShaderMap version is incompatible with SM4Hook " VERSION "\r\nThis is made for " TARGETVERSION, "SM4Hook " VERSION, MB_ICONERROR);
		ExitProcess(-1);
	}
#else
	if (*(WORD*)(hModule + 0x1000) == 0x8D48) //SM4.2.1 x64!
	{
		const std::array<uint8_t, 1> jzToJnZ = { 0x75 };

		WriteProtectedMemory((uintptr_t)(hModule + 0x86F56), jzToJnZ);

		if (_access("_edu_", 0) != -1)
			WriteProtectedMemory((uintptr_t)(hModule + 0x86F94), jzToJnZ);
		else
			WriteProtectedMemory((uintptr_t)(hModule + 0x86F6D), jzToJnZ);
	}
	else
	{
		MessageBox(NULL, "Your ShaderMap version is incompatible with SM4Hook " VERSION "\r\nThis is made for " TARGETVERSION, "SM4Hook " VERSION, MB_ICONERROR);
		ExitProcess(-1);
	}
#endif
}

#ifndef _WIN64
static BYTE originalCode[5];
static PBYTE originalEP = 0;

void Main_UnprotectModule(HMODULE hModule)
{
	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((ADDR_LEN)hModule + header->e_lfanew);

	// unprotect the entire PE image
	SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect((LPVOID)hModule, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

void Main_DoInit()
{
	// return to the original EP
	memcpy(originalEP, &originalCode, sizeof(originalCode));

	// unprotect our entire PE image
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)Main_DoInit, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}

	Sys_RunInit();

	hModule = GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((ADDR_LEN)hModule + header->e_lfanew);

	// back up original code
	originalEP = (PBYTE)((ADDR_LEN)hModule + ntHeader->OptionalHeader.AddressOfEntryPoint);

	__asm jmp originalEP
}

void Main_SetSafeInit()
{
	// find the entry point for the executable process, set page access, and replace the EP
	HMODULE hModule = GetModuleHandle(NULL); // passing NULL should be safe even with the loader lock being held (according to ReactOS ldr.c)

	if (hModule)
	{
		PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
		PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((ADDR_LEN)hModule + header->e_lfanew);

		Main_UnprotectModule(hModule);

		// back up original code
		PBYTE ep = (PBYTE)((ADDR_LEN)hModule + ntHeader->OptionalHeader.AddressOfEntryPoint);
		memcpy(originalCode, ep, sizeof(originalCode));

		// patch to call our EP
		ADDR_LEN newEP = (ADDR_LEN)Main_DoInit - ((ADDR_LEN)ep + 5);
		ep[0] = 0xE9; // for some reason this doesn't work properly when run under the debugger
		memcpy(&ep[1], &newEP, 4);

		originalEP = ep;
	}
}
#endif
bool __stdcall DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
#ifndef _WIN64
		Main_SetSafeInit();
#else
		Sys_RunInit();
#endif
	}

	return true;
}