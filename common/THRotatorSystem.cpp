// (c) 2017 massanoori

#include "stdafx.h"

#include "THRotatorSystem.h"
#include "THRotatorLog.h"
#include "EncodingUtils.h"

namespace
{

double ExtractTouhouSeriesNumber(const boost::filesystem::path& exeFilename)
{
	auto exeFilenameNoExt = exeFilename.stem().generic_wstring();

	if (exeFilenameNoExt.compare(L"東方紅魔郷") == 0)
	{
		return 6.0;
	}

	double touhouIndex = 0.0;

	WCHAR dummy[128];

	int numFilledFields = swscanf_s(exeFilenameNoExt.c_str(), L"th%lf%s", &touhouIndex, dummy, _countof(dummy));
	if (numFilledFields < 1)
	{
		return 0.0;
	}

	while (touhouIndex > 90.0)
	{
		touhouIndex /= 10.0;
	}

	return touhouIndex;
}

class TouhouProcessStats
{
public:
	boost::filesystem::path exeFilePath, exeFilename;
	boost::filesystem::path playerDataDir;
	double touhouSeriesNumber;

	static const TouhouProcessStats& Get()
	{
		static TouhouProcessStats touhouProcessStats;
		static bool bFirstCall = true;

		if (bFirstCall)
		{
			// Don't log anything before bFirstCall becomes false,
			// or infinite recursion occurs.
			bFirstCall = false;

			OutputLogMessagef(LogSeverity::Info, "Player data directory: {0}",
				ConvertFromUnicodeToUtf8(touhouProcessStats.playerDataDir.generic_wstring()));
			OutputLogMessagef(LogSeverity::Info, "Executable filename: {0}",
				ConvertFromUnicodeToUtf8(touhouProcessStats.exeFilePath.generic_wstring()));
		}

		return touhouProcessStats;
	}

private:
	TouhouProcessStats()
	{
		std::vector<WCHAR> exePathRaw;
		exePathRaw.resize(MAX_PATH);

		while (GetModuleFileNameW(NULL, exePathRaw.data(), exePathRaw.size()) == ERROR_INSUFFICIENT_BUFFER)
		{
			exePathRaw.resize(exePathRaw.size() * 2);
		}

		exeFilePath = exePathRaw.data();

		exeFilename = exeFilePath.filename();
		touhouSeriesNumber = ExtractTouhouSeriesNumber(exeFilename);

		WCHAR iniSavePathRaw[MAX_PATH];
		size_t retSize;
		errno_t resultOfGetEnv = _wgetenv_s(&retSize, iniSavePathRaw, L"APPDATA");

		// Since Touhou 12.5, player's data is saved to C:\Users\<username>\AppData\Roaming\ShanghaiAlice\<application>
		if (touhouSeriesNumber > 12.3 && resultOfGetEnv == 0 && retSize > 0)
		{
			playerDataDir = boost::filesystem::path(iniSavePathRaw) / L"ShanghaiAlice" / exeFilename.stem();
			boost::filesystem::create_directory(playerDataDir);
		}
		else
		{
			playerDataDir = boost::filesystem::current_path();
		}
	}
};

HINSTANCE hTHRotatorModule;

} // end of anonymous namespace

double GetTouhouSeriesNumber()
{
	return TouhouProcessStats::Get().touhouSeriesNumber;
}

boost::filesystem::path GetTouhouExecutableFilePath()
{
	return TouhouProcessStats::Get().exeFilePath;
}

boost::filesystem::path GetTouhouExecutableFilename()
{
	return TouhouProcessStats::Get().exeFilename;
}

boost::filesystem::path GetTouhouPlayerDataDirectory()
{
	return TouhouProcessStats::Get().playerDataDir;
}

bool IsTouhouWithoutScreenCapture()
{
	// Touhou 6 is the only title without screen capture in Touhou main series.
	return TouhouProcessStats::Get().touhouSeriesNumber == 6.0;
}

HINSTANCE GetTHRotatorModule()
{
	return hTHRotatorModule;
}

namespace
{

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

} // end of anonymous namespace


BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  fdwReason,
	LPVOID /* lpReserved */)
{
	static HINSTANCE hOriginalDirect3DLibrary;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:

		hTHRotatorModule = reinterpret_cast<HINSTANCE>(hModule);
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
			return FALSE;
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
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		FreeLibrary(hOriginalDirect3DLibrary);
		hOriginalDirect3DLibrary = NULL;
		break;

	default:
		break;
	}
	return TRUE;
}

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
