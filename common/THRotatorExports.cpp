// (c) 2018 massanoori

#include "stdafx.h"

#include "THRotatorExports.h"

namespace
{

HINSTANCE hOriginalDirect3DLibrary;

#ifdef TOUHOU_ON_D3D8

IDirect3D8* (WINAPI * p_Direct3DCreate8)(UINT);

FARPROC p_ValidatePixelShader;
FARPROC p_ValidateVertexShader;
FARPROC p_DebugSetMute;

#else

typedef IDirect3D9* (WINAPI * DIRECT3DCREATE9PROC)(UINT);
typedef HRESULT(WINAPI * DIRECT3DCREATE9EXPROC)(UINT, IDirect3D9Ex**);

FARPROC p_Direct3DShaderValidatorCreate9;
FARPROC p_PSGPError;
FARPROC p_PSGPSampleTexture;
FARPROC p_D3DPERF_BeginEvent;
FARPROC p_D3DPERF_EndEvent;
FARPROC p_D3DPERF_GetStatus;
FARPROC p_D3DPERF_QueryRepeatFrame;
FARPROC p_D3DPERF_SetMarker;
FARPROC p_D3DPERF_SetOptions;
FARPROC p_D3DPERF_SetRegion;
FARPROC p_DebugSetLevel;
FARPROC p_DebugSetMute;
DIRECT3DCREATE9PROC p_Direct3DCreate9;
DIRECT3DCREATE9EXPROC p_Direct3DCreate9Ex;

#endif

} // anonymous-namespace

bool InitializeExports()
{
	{
		TCHAR systemDirectoryRaw[MAX_PATH];
		GetSystemDirectory(systemDirectoryRaw, MAX_PATH);
		boost::filesystem::path systemDirectory(systemDirectoryRaw);

#ifdef TOUHOU_ON_D3D8
		systemDirectory /= L"d3d8.dll";
#else
		systemDirectory /= L"d3d9.dll";
#endif
		hOriginalDirect3DLibrary = LoadLibraryW(systemDirectory.generic_wstring().c_str());
	}

	if (hOriginalDirect3DLibrary == NULL)
	{
		return false;
	}

#ifdef TOUHOU_ON_D3D8
	p_ValidatePixelShader = GetProcAddress(hOriginalDirect3DLibrary, "ValidatePixelShader");
	p_ValidateVertexShader = GetProcAddress(hOriginalDirect3DLibrary, "ValidateVertexShader");
	p_DebugSetMute = GetProcAddress(hOriginalDirect3DLibrary, "DebugSetMute");
	p_Direct3DCreate8 = (IDirect3D8*(WINAPI*)(UINT))GetProcAddress(hOriginalDirect3DLibrary, "Direct3DCreate8");
#else
	p_Direct3DShaderValidatorCreate9 = GetProcAddress(hOriginalDirect3DLibrary, "Direct3DShaderValidatorCreate9");
	p_PSGPError = GetProcAddress(hOriginalDirect3DLibrary, "PSGPError");
	p_PSGPSampleTexture = GetProcAddress(hOriginalDirect3DLibrary, "PSGPSampleTexture");
	p_D3DPERF_BeginEvent = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_BeginEvent");
	p_D3DPERF_EndEvent = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_EndEvent");
	p_D3DPERF_GetStatus = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_GetStatus");
	p_D3DPERF_QueryRepeatFrame = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_QueryRepeatFrame");
	p_D3DPERF_SetMarker = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_SetMarker");
	p_D3DPERF_SetOptions = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_SetOptions");
	p_D3DPERF_SetRegion = GetProcAddress(hOriginalDirect3DLibrary, "D3DPERF_SetRegion");
	p_DebugSetLevel = GetProcAddress(hOriginalDirect3DLibrary, "DebugSetLevel");
	p_DebugSetMute = GetProcAddress(hOriginalDirect3DLibrary, "DebugSetMute");
	p_Direct3DCreate9 = reinterpret_cast<DIRECT3DCREATE9PROC>(GetProcAddress(hOriginalDirect3DLibrary, "Direct3DCreate9"));
	p_Direct3DCreate9Ex = reinterpret_cast<DIRECT3DCREATE9EXPROC>(GetProcAddress(hOriginalDirect3DLibrary, "Direct3DCreate9Ex"));
#endif

	return true;
}

void FinalizeExports()
{
	FreeLibrary(hOriginalDirect3DLibrary);
	hOriginalDirect3DLibrary = NULL;
}

#if TOUHOU_ON_D3D8

IDirect3D8* CallOriginalDirect3DCreate(UINT version)
{
	return p_Direct3DCreate8(version);
}

#else

IDirect3D9* CallOriginalDirect3DCreate(UINT version)
{
	return p_Direct3DCreate9(version);
}

HRESULT CallOriginalDirect3DCreate9Ex(UINT version, IDirect3D9Ex** direct3d)
{
	return p_Direct3DCreate9Ex(version, direct3d);
}

#endif

extern "C"
{
#ifdef TOUHOU_ON_D3D8
	__declspec(naked) void WINAPI d_ValidatePixelShader() { _asm { jmp p_ValidatePixelShader } }
	__declspec(naked) void WINAPI d_ValidateVertexShader() { _asm { jmp p_ValidateVertexShader } }
	__declspec(naked) void WINAPI d_DebugSetMute() { _asm { jmp p_DebugSetMute } }
#else
	__declspec(naked) void WINAPI d_Direct3DShaderValidatorCreate9() { _asm { jmp p_Direct3DShaderValidatorCreate9 } }
	__declspec(naked) void WINAPI d_PSGPError() { _asm { jmp p_PSGPError } }
	__declspec(naked) void WINAPI d_PSGPSampleTexture() { _asm { jmp p_PSGPSampleTexture } }
	__declspec(naked) void WINAPI d_D3DPERF_BeginEvent() { _asm { jmp p_D3DPERF_BeginEvent } }
	__declspec(naked) void WINAPI d_D3DPERF_EndEvent() { _asm { jmp p_D3DPERF_EndEvent } }
	__declspec(naked) void WINAPI d_D3DPERF_GetStatus() { _asm { jmp p_D3DPERF_GetStatus } }
	__declspec(naked) void WINAPI d_D3DPERF_QueryRepeatFrame() { _asm { jmp p_D3DPERF_QueryRepeatFrame } }
	__declspec(naked) void WINAPI d_D3DPERF_SetMarker() { _asm { jmp p_D3DPERF_SetMarker } }
	__declspec(naked) void WINAPI d_D3DPERF_SetOptions() { _asm { jmp p_D3DPERF_SetOptions } }
	__declspec(naked) void WINAPI d_D3DPERF_SetRegion() { _asm { jmp p_D3DPERF_SetRegion } }
	__declspec(naked) void WINAPI d_DebugSetLevel() { _asm { jmp p_DebugSetLevel } }
	__declspec(naked) void WINAPI d_DebugSetMute() { _asm { jmp p_DebugSetMute } }
#endif
}
