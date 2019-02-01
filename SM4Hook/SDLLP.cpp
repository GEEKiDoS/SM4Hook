// --------------------------------------+
// System Dynamic Link Library Proxy
// by momo5502
// --------------------------------------+
#include "stdafx.h"

// Static class
// --------------------------------------+
class SDLLP
{
private:
	static std::map<std::string, HINSTANCE> mLibraries;

	static void	Log(const char* message, ...);
	static void	__fastcall LoadLibrary(const char* library);
	static bool	__fastcall IsLibraryLoaded(const char* library);

public:
	static FARPROC __fastcall GetExport(const char* function, const char* library);
};

//	Class variable declarations
// --------------------------------------+
std::map<std::string, HINSTANCE> SDLLP::mLibraries;

// Load necessary library
// --------------------------------------+
void __fastcall SDLLP::LoadLibrary(const char* library)
{
	Log("[SDLLP] Loading library '%s'.", library);

	CHAR mPath[MAX_PATH];

	GetSystemDirectoryA(mPath, MAX_PATH);
	strcat_s(mPath, "\\");
	strcat_s(mPath, library);

	mLibraries[library] = ::LoadLibraryA(mPath);

	if (!IsLibraryLoaded(library)) Log("[SDLLP] Unable to load library '%s'.", library);
}

// Check if export already loaded
// --------------------------------------+
bool __fastcall SDLLP::IsLibraryLoaded(const char* library)
{
	return (mLibraries.find(library) != mLibraries.end() && mLibraries[library]);
}

// Get export address
// --------------------------------------+
FARPROC __fastcall SDLLP::GetExport(const char* function, const char* library)
{
	Log("[SDLLP] Export '%s' requested from '%s'.", function, library);

	if (!IsLibraryLoaded(library)) LoadLibrary(library);

	FARPROC address = GetProcAddress(mLibraries[library], function);

	if (!address) Log("[SDLLP] Unable to load export '%s' from library '%s'.", function, library);
	return address;
}

// Write debug string
// --------------------------------------+
void SDLLP::Log(const char* message, ...)
{
	CHAR buffer[1024];
	va_list ap;

	va_start(ap, message);
	vsprintf_s(buffer, message, ap);
	va_end(ap);

	OutputDebugStringA(buffer);
}

#ifdef _WIN64
#include "d3d9.h"
extern "C"
{
	typedef IDirect3D9*(*Direct3DCreate9_f)(UINT);

	__declspec(dllexport) IDirect3D9 * WINAPI Direct3DCreate9(UINT SDKVersion)
	{
		static Direct3DCreate9_f function = 0;
		if (!function) 
			function = (Direct3DCreate9_f)SDLLP::GetExport(__FUNCTION__, "D3D9.dll");

		return function(SDKVersion);
	}
}
#else

// Macro to declare an export
// --------------------------------------+
#define EXPORT(_export) extern "C" __declspec(naked) __declspec(dllexport) void _export() { static FARPROC function = 0; if(!function) function = SDLLP::GetExport(__FUNCTION__, LIBRARY); __asm { jmp function } }  

// --------------------------------------+
//	Adapt export functions and library
// --------------------------------------+

#define LIBRARY "D3D9.dll"
EXPORT(Direct3DCreate9)
#endif
