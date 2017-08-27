// (c) 2017 massanoori

#include "stdafx.h"
#include "StringResource.h"
#include "EncodingUtils.h"

std::string LoadTHRotatorStringUtf8(HINSTANCE hModule, UINT nID)
{
	return ConvertFromUnicodeToUtf8(LoadTHRotatorStringUnicode(hModule, nID));
}

std::wstring LoadTHRotatorStringUnicode(HINSTANCE hModule, UINT nID)
{
	LPWSTR temp;
	auto bufferLength = LoadStringW(hModule, nID, reinterpret_cast<LPWSTR>(&temp), 0);

	if (bufferLength == 0)
	{
		return std::wstring();
	}
	else
	{
		return std::wstring(temp, bufferLength);
	}
}
