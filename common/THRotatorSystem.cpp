// (c) 2017 massanoori

#include "stdafx.h"

#include "THRotatorSystem.h"
#include "THRotatorLog.h"
#include "EncodingUtils.h"
#include "THRotatorExports.h"

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

	int numFilledFields = swscanf_s(exeFilenameNoExt.c_str(), L"th%lf%s", &touhouIndex, dummy, static_cast<std::uint32_t>(_countof(dummy)));
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

		while (GetModuleFileNameW(NULL, exePathRaw.data(), static_cast<DWORD>(exePathRaw.size())) == ERROR_INSUFFICIENT_BUFFER)
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

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  fdwReason,
	LPVOID /* lpReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hTHRotatorModule = reinterpret_cast<HINSTANCE>(hModule);
		if (!InitializeExports())
		{
			OutputLogMessage(LogSeverity::Error, "Failed to initialize exported function of Direct3D.");
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		FinalizeExports();
		hTHRotatorModule = NULL;
		break;

	default:
		break;
	}
	return TRUE;
}
