// (c) 2017 massanoori

#include "stdafx.h"

#include "EncodingUtils.h"

std::wstring ConvertFromSjisToUnicode(const std::string & from)
{
	int unicodeBufferSize = ::MultiByteToWideChar(CP_ACP, 0, from.c_str(), -1, nullptr, 0);
	std::unique_ptr<WCHAR[]> unicodeChar(new WCHAR[unicodeBufferSize]);
	::MultiByteToWideChar(CP_ACP, 0, from.c_str(), -1, unicodeChar.get(), unicodeBufferSize);

	return std::wstring(unicodeChar.get());
}

std::string ConvertFromUnicodeToSjis(const std::wstring & from)
{
	int utf8BufferSize = ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<CHAR[]> sjisChar(new CHAR[utf8BufferSize]);
	::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, sjisChar.get(), utf8BufferSize, nullptr, nullptr);

	return std::string(sjisChar.get());
}

std::string ConvertFromUnicodeToUtf8(const std::wstring & from)
{
	int utf8BufferSize = ::WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<CHAR[]> utf8Char(new CHAR[utf8BufferSize]);
	::WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, utf8Char.get(), utf8BufferSize, nullptr, nullptr);

	return std::string(utf8Char.get());
}

std::wstring ConvertFromUtf8ToUnicode(const std::string & from)
{
	int unicodeBufferSize = ::MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, nullptr, 0);
	std::unique_ptr<WCHAR[]> unicodeChar(new WCHAR[unicodeBufferSize]);
	::MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, unicodeChar.get(), unicodeBufferSize);

	return std::wstring(unicodeChar.get());
}

std::string ConvertFromSjisToUtf8(const std::string & from)
{
	std::wstring unicodeStr = ConvertFromSjisToUnicode(from);
	return ConvertFromUnicodeToUtf8(unicodeStr);
}

std::string ConvertFromUtf8ToSjis(const std::string & from)
{
	std::wstring unicodeStr = ConvertFromUtf8ToUnicode(from);
	return ConvertFromUnicodeToSjis(unicodeStr);
}
