// (c) 2017 massanoori

#pragma once

std::wstring ConvertFromSjisToUnicode(const std::string& from);
std::string ConvertFromUnicodeToSjis(const std::wstring& from);
std::string ConvertFromUnicodeToUtf8(const std::wstring& from);
std::wstring ConvertFromUtf8ToUnicode(const std::string& from);
std::string ConvertFromSjisToUtf8(const std::string& from);
std::string ConvertFromUtf8ToSjis(const std::string& from);
