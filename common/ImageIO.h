// (c) 2017 massanoori

#pragma once

/**
 * Save pixel data to bitmap file.
 *
 * \param format           Pixel format. Acceptable formats are D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_R5G6B5, and D3DFMT_R8G8B8.
 * \param pData            Pixel data.
 * \param width            Image width.
 * \param height           Image height.
 * \param linePitchInBytes Size of pixels pointed by pData per one scanline in bytes.
 *
 * \return true when successful. false otherwise. Reason for failure is logged.
 */
bool SaveBitmapImage(const std::wstring& filename, D3DFORMAT format, const void* pData,
	std::uint32_t width, std::uint32_t height, std::uint32_t linePitchInBytes);
