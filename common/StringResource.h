// (c) 2017 massanoori

#pragma once

/**
 * Load string from string table resource (UTF-8).
 */
std::string LoadTHRotatorStringUtf8(HINSTANCE hModule, UINT nID);

/**
 * Load string from string table resource (Unicode).
 */
std::wstring LoadTHRotatorStringUnicode(HINSTANCE hModule, UINT nID);
