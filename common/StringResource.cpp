// (c) 2017 massanoori

#include "stdafx.h"
#include "StringResource.h"

std::wstring LoadTHRotatorString(HINSTANCE hModule, UINT nID)
{
	LPWSTR temp;
	auto bufferLength = LoadStringW(hModule, nID, reinterpret_cast<LPWSTR>(&temp), 0);

	if (bufferLength == 0)
	{
		return std::basic_string<TCHAR>();
	}
	else
	{
		return std::wstring(temp, bufferLength);
	}
}
