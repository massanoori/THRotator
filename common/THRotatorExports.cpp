// (c) 2018 massanoori

#include "stdafx.h"

#include "THRotatorExports.h"
#include "THRotatorDirect3D.h"
#include "THRotatorLog.h"

enum ExportedFuncIndex
{
#ifdef TOUHOU_ON_D3D8
	ExportedFuncIndex_ValidatePixelShader,
	ExportedFuncIndex_ValidateVertexShader,
	ExportedFuncIndex_DebugSetMute,
	ExportedFuncIndex_Direct3DCreate8,
#else
	ExportedFuncIndex_Direct3DShaderValidatorCreate9,
	ExportedFuncIndex_PSGPError,
	ExportedFuncIndex_PSGPSampleTexture,
	ExportedFuncIndex_D3DPERF_BeginEvent,
	ExportedFuncIndex_D3DPERF_EndEvent,
	ExportedFuncIndex_D3DPERF_GetStatus,
	ExportedFuncIndex_D3DPERF_QueryRepeatFrame,
	ExportedFuncIndex_D3DPERF_SetMarker,
	ExportedFuncIndex_D3DPERF_SetOptions,
	ExportedFuncIndex_D3DPERF_SetRegion,
	ExportedFuncIndex_DebugSetLevel,
	ExportedFuncIndex_DebugSetMute,
	ExportedFuncIndex_Direct3DCreate9,
	ExportedFuncIndex_Direct3DCreate9Ex,
#endif

	NumExportedFuncIndices,
};

extern "C" UINT_PTR exportedFunctions[NumExportedFuncIndices]{};

namespace
{

HINSTANCE hOriginalDirect3DLibrary;

void SetExportedFunction(UINT functionIndex, const char* functionName)
{
	exportedFunctions[functionIndex] = reinterpret_cast<UINT_PTR>(GetProcAddress(hOriginalDirect3DLibrary, functionName));
}

FARPROC GetExportedFunction(UINT functionIndex)
{
	return reinterpret_cast<FARPROC>(exportedFunctions[functionIndex]);
}

} // anonymous-namespace

bool InitializeExports()
{
	TCHAR systemDirectoryRaw[MAX_PATH];
	GetSystemDirectory(systemDirectoryRaw, MAX_PATH);
	std::filesystem::path originalDirect3DLibraryPath(systemDirectoryRaw);

#ifdef TOUHOU_ON_D3D8
	originalDirect3DLibraryPath /= L"d3d8.dll";
#else
	originalDirect3DLibraryPath /= L"d3d9.dll";
#endif

	hOriginalDirect3DLibrary = LoadLibraryW(originalDirect3DLibraryPath.generic_wstring().c_str());

	if (hOriginalDirect3DLibrary == NULL)
	{
		return false;
	}

#define SET_EXPORTED_FUNCTION_SHORTCUT(name) do { \
	SetExportedFunction(ExportedFuncIndex_##name, #name); \
	if (GetExportedFunction(ExportedFuncIndex_##name) == nullptr) \
	{ \
		OutputLogMessagef(LogSeverity::Warning, "\"{0}\" doesn't export \"{1}\"", originalDirect3DLibraryPath.generic_string().c_str(), #name); \
	} } while(false)

#ifdef TOUHOU_ON_D3D8
	SET_EXPORTED_FUNCTION_SHORTCUT(ValidatePixelShader);
	SET_EXPORTED_FUNCTION_SHORTCUT(ValidateVertexShader);
	SET_EXPORTED_FUNCTION_SHORTCUT(DebugSetMute);
	SET_EXPORTED_FUNCTION_SHORTCUT(Direct3DCreate8);
#else
	SET_EXPORTED_FUNCTION_SHORTCUT(Direct3DShaderValidatorCreate9);
	SET_EXPORTED_FUNCTION_SHORTCUT(PSGPError);
	SET_EXPORTED_FUNCTION_SHORTCUT(PSGPSampleTexture);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_BeginEvent);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_EndEvent);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_GetStatus);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_QueryRepeatFrame);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_SetMarker);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_SetOptions);
	SET_EXPORTED_FUNCTION_SHORTCUT(D3DPERF_SetRegion);
	SET_EXPORTED_FUNCTION_SHORTCUT(DebugSetLevel);
	SET_EXPORTED_FUNCTION_SHORTCUT(DebugSetMute);
	SET_EXPORTED_FUNCTION_SHORTCUT(Direct3DCreate9);
	SET_EXPORTED_FUNCTION_SHORTCUT(Direct3DCreate9Ex);
#endif

#undef SET_EXPORTED_FUNCTION_SHORTCUT

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
	using Signature_Direct3DCreate8 = IDirect3D8 * (WINAPI*)(UINT);

	if (auto pDirect3DCreate8 = reinterpret_cast<Signature_Direct3DCreate8>(GetExportedFunction(ExportedFuncIndex_Direct3DCreate8)))
	{
		return pDirect3DCreate8(version);
	}
	
	OutputLogMessagef(LogSeverity::Error, "{0} is not callable because the original {1} exports {0}", "Direct3DCreate8", "d3d8.dll");
	return nullptr;
}

#else

IDirect3D9* CallOriginalDirect3DCreate(UINT version)
{
	using Signature_Direct3DCreate9 = IDirect3D9 * (WINAPI*)(UINT);

	if (auto pDirect3DCreate9 = reinterpret_cast<Signature_Direct3DCreate9>(GetExportedFunction(ExportedFuncIndex_Direct3DCreate9)))
	{
		return pDirect3DCreate9(version);
	}

	OutputLogMessagef(LogSeverity::Error, "{0} is not callable because the original {1} exports {0}", "Direct3DCreate9", "d3d9.dll");
	return nullptr;
}

HRESULT CallOriginalDirect3DCreate9Ex(UINT version, IDirect3D9Ex** direct3d)
{
	using Signature_Direct3DCreate9Ex = HRESULT(WINAPI*)(UINT, IDirect3D9Ex**);

	if (auto pDirect3DCreate9Ex = reinterpret_cast<Signature_Direct3DCreate9Ex>(GetExportedFunction(ExportedFuncIndex_Direct3DCreate9Ex)))
	{
		return pDirect3DCreate9Ex(version, direct3d);
	}

	OutputLogMessagef(LogSeverity::Error, "{0} is not callable because the original {1} exports {0}", "Direct3DCreate9Ex", "d3d9.dll");
	return D3DERR_NOTAVAILABLE;
}

#endif

#define EXPORTED_FUNCTION_HEADER_BASE(name, declspec_arg, return_type) extern "C" __declspec(declspec_arg) return_type WINAPI d_##name

#ifdef _WIN64
// On x64, definitions are in .asm
#define DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(name) EXPORTED_FUNCTION_HEADER_BASE(name, , void)();
#else
// On x86, call directly
#define DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(name) EXPORTED_FUNCTION_HEADER_BASE(name, naked, void)() { _asm { jmp exportedFunctions[4 * ExportedFuncIndex_##name] } }
#endif

#define EXPORTED_FUNCTION_HEADER(name, return_type) EXPORTED_FUNCTION_HEADER_BASE(name, dllexport, return_type)

#ifdef TOUHOU_ON_D3D8

// Exported function definitions for Direct3D 8

DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(ValidatePixelShader)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(ValidateVertexShader)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(DebugSetMute)

EXPORTED_FUNCTION_HEADER(Direct3DCreate8, Direct3DBase*)(UINT version)
{
	return THRotatorDirect3DCreate(version);
}

#else

// Exported function definitions for Direct3D 9

DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(Direct3DShaderValidatorCreate9)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(PSGPError)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(PSGPSampleTexture)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_BeginEvent)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_EndEvent)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_GetStatus)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_QueryRepeatFrame)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_SetMarker)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_SetOptions)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(D3DPERF_SetRegion)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(DebugSetLevel)
DEFINE_EXPORTED_FUNCTION_PASSTHROUGH(DebugSetMute)

EXPORTED_FUNCTION_HEADER(Direct3DCreate9, Direct3DBase*)(UINT version)
{
	return THRotatorDirect3DCreate(version);
}

EXPORTED_FUNCTION_HEADER(Direct3DCreate9Ex, HRESULT)(UINT version, Direct3DExBase** ppD3D)
{
	return THRotatorDirect3DCreateEx(version, ppD3D);
}

#endif
