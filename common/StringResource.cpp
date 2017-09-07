// (c) 2017 massanoori

#include "stdafx.h"
#include "StringResource.h"
#include "EncodingUtils.h"
#include "THRotatorSystem.h"

std::string LoadTHRotatorStringUtf8(UINT nID)
{
	return ConvertFromUnicodeToUtf8(LoadTHRotatorStringUnicode(nID));
}

std::wstring LoadTHRotatorStringUnicode(UINT nID)
{
	LPWSTR temp;
	auto bufferLength = LoadStringW(GetTHRotatorModule(), nID, reinterpret_cast<LPWSTR>(&temp), 0);

	if (bufferLength == 0)
	{
		return std::wstring();
	}
	else
	{
		return std::wstring(temp, bufferLength);
	}
}
