// (c) 2017 massanoori

#pragma once

/**
 * Creators of Direct3D interfaces.
 */

#if TOUHOU_ON_D3D8

// Direct3D 8
IDirect3D8* CallOriginalDirect3DCreate(UINT version);
#else

// Direct3D 9
IDirect3D9* CallOriginalDirect3DCreate(UINT version);

// Direct3D 9 Ex
HRESULT CallOriginalDirect3DCreate9Ex(UINT version, IDirect3D9Ex** direct3d);
#endif

/**
 * Get module handle of THRotator's DLL.
 */
HINSTANCE GetTHRotatorModule();

/**
 * Get touhou series number of current process in real number.
 * Examples:
 *   東方紅魔郷 (EoSD): 6.0
 *   東方文花帖 (StB): 9.5
 *   東方地霊殿 (SA): 11.0
 *   妖精大戦争 (FW): 12.8
 *
 * Filename of Touhou executable is "th<number><tr if demo>.exe" or "東方紅魔郷.exe".
 * This function extracts series number from executable filename.
 * If the rule above is not applied, 0.0 is returned.
 */
double GetTouhouSeriesNumber();

/**
 * Get full path of executable.
 */
boost::filesystem::path GetTouhouExecutableFilePath();

/**
 * Get filename of executable. Directory is excluded.
 */
boost::filesystem::path GetTouhouExecutableFilename();

/**
 * Get directory where player's data is saved.
 */
boost::filesystem::path GetTouhouPlayerDataDirectory();

/**
 * Returns true if current Touhou doesn't support native screen capture.
 *
 * As of 2017, 東方紅魔郷 (EoSD) only.
 */
bool IsTouhouWithoutScreenCapture();
