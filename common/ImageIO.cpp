// (c) 2017 massanoori

#include "stdafx.h"
#include "ImageIO.h"
#include "THRotatorLog.h"
#include "EncodingUtils.h"

#include <fstream>

namespace
{

static_assert(sizeof(RGBTRIPLE) == 3, "");

/**
 * Writing pixels to output stream.
 */
template <typename LinePixelIteratorType>
void SaveBitmapPixels24(std::ostream& ofs, const void* pData,
	std::uint32_t width, std::uint32_t height, std::uint32_t linePitchInBytes)
{
	std::uint32_t numWrittenDwordsPerLine = (3u * width + sizeof(DWORD) - 1) / sizeof(DWORD);

	std::unique_ptr<DWORD[]> lineDataBuffer(new DWORD[numWrittenDwordsPerLine]);
	ZeroMemory(lineDataBuffer.get(), sizeof(DWORD) * numWrittenDwordsPerLine);

	for (std::uint32_t y = 0; y < height; y++)
	{
		auto pInputLineData = static_cast<const std::uint8_t*>(pData) + (height - y - 1) * linePitchInBytes;
		auto pWrittenLineData = reinterpret_cast<RGBTRIPLE*>(lineDataBuffer.get());

		LinePixelIteratorType linePixelIterator(pInputLineData);

		for (std::uint32_t x = 0; x < width; x++, ++linePixelIterator)
		{
			pWrittenLineData[x] = *linePixelIterator;
		}

		ofs.write(reinterpret_cast<const char*>(lineDataBuffer.get()), sizeof(DWORD) * numWrittenDwordsPerLine);
	}
}

// Pixel iterators for one scanline starting from a pointer specified as a ctor argument.

struct LinePixelIteratorR8G8B8_ToRGB24
{
	const std::uint8_t* pData;

	LinePixelIteratorR8G8B8_ToRGB24(const void* p)
		: pData(static_cast<const std::uint8_t*>(p))
	{
	}

	LinePixelIteratorR8G8B8_ToRGB24& operator++()
	{
		pData += 3;
		return *this;
	}

	RGBTRIPLE operator*() const
	{
		return RGBTRIPLE{ pData[0], pData[1], pData[2] };
	}
};

struct LinePixelIteratorX8R8G8B8_ToRGB24
{
	const std::uint32_t* pData;

	LinePixelIteratorX8R8G8B8_ToRGB24(const void* p)
		: pData(static_cast<const std::uint32_t*>(p))
	{
	}

	LinePixelIteratorX8R8G8B8_ToRGB24& operator++()
	{
		pData++;
		return *this;
	}

	RGBTRIPLE operator*() const
	{
		return RGBTRIPLE
		{
			static_cast<BYTE>((*pData >>  0) & 0xff),	
			static_cast<BYTE>((*pData >>  8) & 0xff),
			static_cast<BYTE>((*pData >> 16) & 0xff),
		};
	}
};

struct LinePixelIteratorR5G6B5_ToRGB24
{
	const std::uint16_t* pData;

	LinePixelIteratorR5G6B5_ToRGB24(const void* p)
		: pData(static_cast<const std::uint16_t*>(p))
	{
	}

	LinePixelIteratorR5G6B5_ToRGB24& operator++()
	{
		pData++;
		return *this;
	}

	RGBTRIPLE operator*() const
	{
		return RGBTRIPLE
		{
			static_cast<BYTE>(((*pData >>  0) & 0x1f) * 255.0f / 31.0f),
			static_cast<BYTE>(((*pData >>  5) & 0x3f) * 255.0f / 63.0f),
			static_cast<BYTE>(((*pData >> 11) & 0x1f) * 255.0f / 31.0f),
		};
	}
};

}

bool SaveBitmapImage(const std::wstring& filename, D3DFORMAT format, const void* pData,
	std::uint32_t width, std::uint32_t height, std::uint32_t linePitchInBytes)
{
	if (format != D3DFMT_X8R8G8B8 &&
		format != D3DFMT_A8R8G8B8 &&
		format != D3DFMT_R5G6B5 &&
		format != D3DFMT_R8G8B8)
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to save screen shot to \"{0}\": unsupported pixel format \"{1}\".",
			ConvertFromUnicodeToUtf8(filename), format);
		return false;
	}

	BITMAPFILEHEADER fileHeader;
	ZeroMemory(&fileHeader, sizeof(fileHeader));
	fileHeader.bfType = *reinterpret_cast<const std::uint16_t*>("BM");
	fileHeader.bfSize = sizeof(fileHeader);

	BITMAPCOREHEADER coreHeader;
	ZeroMemory(&coreHeader, sizeof(coreHeader));
	coreHeader.bcSize = sizeof(coreHeader);
	coreHeader.bcWidth = static_cast<WORD>(width);
	coreHeader.bcHeight = static_cast<WORD>(height);
	coreHeader.bcPlanes = 1;
	coreHeader.bcBitCount = 24;

	// pixel data starts just after bitmap headers.
	fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(coreHeader);

	std::ofstream ofs(filename, std::ios::binary | std::ios::out);

	if (ofs.fail() || ofs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to save screen shot to \"{0}\" failed: file IO failed.",
			ConvertFromUnicodeToUtf8(filename));
		return false;
	}

	ofs.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
	ofs.write(reinterpret_cast<const char*>(&coreHeader), sizeof(coreHeader));

	switch (format)
	{
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8R8G8B8:
		SaveBitmapPixels24<LinePixelIteratorX8R8G8B8_ToRGB24>(ofs, pData, width, height, linePitchInBytes);
		break;

	case D3DFMT_R5G6B5:
		SaveBitmapPixels24<LinePixelIteratorR5G6B5_ToRGB24>(ofs, pData, width, height, linePitchInBytes);
		break;

	case D3DFMT_R8G8B8:
		SaveBitmapPixels24<LinePixelIteratorR8G8B8_ToRGB24>(ofs, pData, width, height, linePitchInBytes);
		break;
	}

	return true;
}
