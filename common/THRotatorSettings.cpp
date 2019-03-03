// (c) 2017 massanoori

#include "stdafx.h"

#include <regex>
#include <fstream>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "THRotatorSettings.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"

namespace
{

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
void ReadIniValue(const std::unordered_map<std::string, std::string>& valueMap, const std::string& key, T& out)
{
	try
	{
		const auto& value = valueMap.at(key);
		try
		{
			std::size_t numConvertedChars = 0u;
			out = static_cast<T>(std::stoi(value, &numConvertedChars));

			if (numConvertedChars < value.size())
			{
				throw std::invalid_argument("");
			}
		}
		catch (const std::invalid_argument&)
		{
			OutputLogMessagef(LogSeverity::Warning, "Value '{0}' at key '{1}' cannot be converted to integer, parameter is initialized with default value", value, key);
		}
	}
	catch (const std::out_of_range&)
	{
		OutputLogMessagef(LogSeverity::Warning, "Key '{0}' not found, parameter is initialized with default value", key);
	}
}

void ReadIniValue(const std::unordered_map<std::string, std::string>& valueMap, const std::string& key, std::string& out)
{
	try
	{
		const auto& value = valueMap.at(key);
		out = value;
	}
	catch (const std::out_of_range&)
	{
		OutputLogMessagef(LogSeverity::Warning, "Key '{0}' not found, parameter is initialized with default value", key);
	}
}

void ReadIniValue(const std::unordered_map<std::string, std::string>& valueMap, const std::string& key, RotationAngle& out)
{
	try
	{
		const auto& value = valueMap.at(key);
		try
		{
			std::size_t numConvertedChars = 0u;
			auto candidate = static_cast<RotationAngle>(std::stoi(value, &numConvertedChars));

			if (numConvertedChars < value.size())
			{
				throw std::invalid_argument("");
			}

			if (candidate < Rotation_Num)
			{
				out = candidate;
			}
			else
			{
				OutputLogMessagef(LogSeverity::Warning, "Value '{0}' at key '{1}' is not in valid range [0, 3], parameter is initialized with default value", value, key);
			}
			
		}
		catch (const std::invalid_argument&)
		{
			OutputLogMessagef(LogSeverity::Warning, "Value '{0}' at key '{1}' cannot be converted to integer, parameter is initialized with default value", value, key);
		}
	}
	catch (const std::out_of_range&)
	{
		OutputLogMessagef(LogSeverity::Warning, "Key '{0}' not found, parameter is initialized with default value", key);
	}
}

void ReadIniValue(const std::unordered_map<std::string, std::string>& valueMap, const std::string& key, D3DTEXTUREFILTERTYPE& out)
{
	try
	{
		const auto& value = valueMap.at(key);
		try
		{
			std::size_t numConvertedChars = 0u;
			auto candidate = static_cast<D3DTEXTUREFILTERTYPE>(std::stoi(value, &numConvertedChars));

			if (numConvertedChars < value.size())
			{
				throw std::invalid_argument("");
			}

			if (candidate == D3DTEXF_NONE || candidate == D3DTEXF_LINEAR)
			{
				out = candidate;
			}
			else
			{
				OutputLogMessagef(LogSeverity::Warning, "Value '{0}' at key '{1}' must be {2} or {3}, parameter is initialized with default value", value, key,
					static_cast<std::underlying_type_t<D3DTEXTUREFILTERTYPE>>(D3DTEXF_NONE),
					static_cast<std::underlying_type_t<D3DTEXTUREFILTERTYPE>>(D3DTEXF_LINEAR));
			}
		}
		catch (const std::invalid_argument&)
		{
			OutputLogMessagef(LogSeverity::Warning, "Value '{0}' at key '{1}' cannot be converted to integer (given value: {1}), parameter is initialized with default value", value, key);
		}
	}
	catch (const std::out_of_range&)
	{
		OutputLogMessagef(LogSeverity::Warning, "Key '{0}' not found, parameter is initialized with default value", key);
	}
}

template <typename T>
void ReadIndexedIniValue(const std::unordered_map<std::string, std::string>& valueMap, const std::string& key, uint32_t index, T& out)
{
	ReadIniValue(valueMap, fmt::format("{0}{1}", key, index), out);
}

template <typename T>
void ReadJsonObjectValue(const nlohmann::json& j, const std::string& key, T& out)
{
	try
	{
		out = j.at(key);
	}
	catch (const std::domain_error& e)
	{
		throw std::domain_error(fmt::format("Failed to parse key '{0}' ({1})", key, e.what()));
	}
}

std::string GenerateIniAppName()
{
	return ConvertFromUnicodeToSjis(std::wstring(L"THRotator_") + GetTouhouExecutableFilename().stem().generic_wstring());
}

std::filesystem::path CreateIniFilePath()
{
	return GetTouhouPlayerDataDirectory() / L"throt.ini";
}

std::filesystem::path CreateJsonFilePath()
{
	auto jsonPath = GetTouhouPlayerDataDirectory() / GetTouhouExecutableFilename();
	jsonPath += L".throtator";
	return jsonPath;
}

}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, RotationAngle& a)
{
	using UnderlyingType = typename std::underlying_type<RotationAngle>::type;

	UnderlyingType raw;
	nlohmann::from_json(j, raw);

	if (raw < 0 || raw >= Rotation_Num)
	{
		throw std::domain_error("rotation angle must be 0, 1, 2, or 3");
	}

	a = static_cast<RotationAngle>(raw);
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, THRotatorFormatVersion& v)
{
	using UnderlyingType = typename std::underlying_type<THRotatorFormatVersion>::type;

	auto minimalVersion = static_cast<UnderlyingType>(THRotatorFormatVersion::Version_2);
	auto maximalVersion = static_cast<UnderlyingType>(THRotatorFormatVersion::Latest);

	UnderlyingType raw;
	nlohmann::from_json(j, raw);

	if (raw < minimalVersion)
	{
		throw std::domain_error(fmt::format("minimal accepted format version is {0}, but is {1}", minimalVersion, raw));
	}

	if (raw > maximalVersion)
	{
		throw std::domain_error(fmt::format("latest format version is {0}, but is {1}", maximalVersion, raw));
	}

	v = static_cast<THRotatorFormatVersion>(raw);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, THRotatorFormatVersion v)
{
	using UnderlyingType = typename std::underlying_type<THRotatorFormatVersion>::type;
	nlohmann::to_json(j, static_cast<UnderlyingType>(v));
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, D3DTEXTUREFILTERTYPE& filterType)
{
	std::string raw;
	nlohmann::from_json(j, raw);

	if (raw != "linear" && raw != "none")
	{
		throw std::domain_error(fmt::format("filter type must be either 'linear' or 'none', but is {0}", raw));
	}

	if (raw == "none")
	{
		filterType = D3DTEXF_NONE;
	}
	else
	{
		filterType = D3DTEXF_LINEAR;
	}
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, D3DTEXTUREFILTERTYPE filterType)
{
	assert(filterType == D3DTEXF_LINEAR || filterType == D3DTEXF_NONE);

	if (filterType == D3DTEXF_NONE)
	{
		nlohmann::to_json(j, "none");
	}
	else
	{
		nlohmann::to_json(j, "linear");
	}
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, RectTransferData& rectData)
{
	if (!j.is_object())
	{
		throw std::domain_error(fmt::format("type must be object, but is {0}", j.type_name()));
	}

	std::string rectName;
	ReadJsonObjectValue(j, "rect_name", rectName);

	rectData.name = rectName;

	ReadJsonObjectValue(j, "source_left", rectData.sourcePosition.x);
	ReadJsonObjectValue(j, "source_top", rectData.sourcePosition.y);
	ReadJsonObjectValue(j, "source_width", rectData.sourceSize.cx);
	ReadJsonObjectValue(j, "source_height", rectData.sourceSize.cy);
	
	ReadJsonObjectValue(j, "destination_left", rectData.destPosition.x);
	ReadJsonObjectValue(j, "destination_top", rectData.destPosition.y);
	ReadJsonObjectValue(j, "destination_width", rectData.destSize.cx);
	ReadJsonObjectValue(j, "destination_height", rectData.destSize.cy);

	ReadJsonObjectValue(j, "rect_rotation", rectData.rotation);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const RectTransferData& rectData)
{
	j["rect_name"] = rectData.name;

	j["source_left"] = rectData.sourcePosition.x;
	j["source_top"] = rectData.sourcePosition.y;
	j["source_width"] = rectData.sourceSize.cx;
	j["source_height"] = rectData.sourceSize.cy;

	j["destination_left"] = rectData.destPosition.x;
	j["destination_top"] = rectData.destPosition.y;
	j["destination_width"] = rectData.destSize.cx;
	j["destination_height"] = rectData.destSize.cy;

	j["rect_rotation"] = rectData.rotation;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, THRotatorSetting& setting)
{
	if (!j.is_object())
	{
		OutputLogMessagef(LogSeverity::Error, "type must be object, but is {0}", j.type_name());
		return;
	}

	// used for migration
	THRotatorFormatVersion formatVersion;
	ReadJsonObjectValue(j, "format_version", formatVersion);

	ReadJsonObjectValue(j, "judge_threshold", setting.judgeThreshold);
	ReadJsonObjectValue(j, "main_screen_left", setting.mainScreenTopLeft.x);
	ReadJsonObjectValue(j, "main_screen_top", setting.mainScreenTopLeft.y);
	ReadJsonObjectValue(j, "main_screen_width", setting.mainScreenSize.cx);
	ReadJsonObjectValue(j, "main_screen_height", setting.mainScreenSize.cy);
	ReadJsonObjectValue(j, "y_offset", setting.yOffset);
	ReadJsonObjectValue(j, "window_visible", setting.bVisible);
	ReadJsonObjectValue(j, "vertical_window", setting.bVerticallyLongWindow);
	ReadJsonObjectValue(j, "rotation_angle", setting.rotationAngle);
	ReadJsonObjectValue(j, "fileter_type", setting.filterType);

	auto rectsItr = j.find("rects");
	if (rectsItr == j.end() || !rectsItr->is_array())
	{
		setting.rectTransfers.clear();
		return;
	}

	auto rectsObject = *rectsItr;

	setting.rectTransfers.reserve(rectsObject.size());
	int currentRectIndex = 0;
	for (const auto& rectObject : rectsObject)
	{
		RectTransferData rectData = rectObject;
		setting.rectTransfers.push_back(rectData);

		currentRectIndex++;
	}
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const THRotatorSetting& setting)
{
	j["format_version"] = THRotatorFormatVersion::Latest;
	j["judge_threshold"] = setting.judgeThreshold;
	j["main_screen_left"] = setting.mainScreenTopLeft.x;
	j["main_screen_top"] = setting.mainScreenTopLeft.y;
	j["main_screen_width"] = setting.mainScreenSize.cx;
	j["main_screen_height"] = setting.mainScreenSize.cy;
	j["y_offset"] = setting.yOffset;
	j["window_visible"] = setting.bVisible;
	j["vertical_window"] = setting.bVerticallyLongWindow;
	j["fileter_type"] = setting.filterType;
	j["rotation_angle"] = setting.rotationAngle;

	nlohmann::json rectsArray(nlohmann::json::value_t::array);

	for (const auto& rectData : setting.rectTransfers)
	{
		nlohmann::json rectObject = rectData;
		rectsArray.push_back(rectObject);
	}

	j["rects"] = rectsArray;
}

void THRotatorSetting::LoadIniFormat(THRotatorSetting& outSetting)
{
	auto filename = CreateIniFilePath();

	OutputLogMessagef(LogSeverity::Info, "Loading {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));

	std::ifstream ifs;
	ifs.open(filename.generic_wstring());

	if (ifs.fail() || ifs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to load {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));
		throw std::ios::failure("");
	}

	static std::regex sectionPattern("\\s*\\[\\s*(.*)\\s*\\]\\s*");
	static std::regex keyValuePattern("\\s*(\\w+)\\s*=\\s*(.*)\\s*");

	auto throtatorSectionName = GenerateIniAppName(); // Referenced by READ_INI_PARAM macro

	std::string line;
	bool bInTHRotatorSection = false;
	std::unordered_map<std::string, std::string> valueMap;
	while (std::getline(ifs, line))
	{
		if (!bInTHRotatorSection)
		{
			std::smatch match;
			if (!std::regex_match(line, match, sectionPattern))
			{
				continue;
			}

			const auto& sectionName = match.str(1);

			if (sectionName == throtatorSectionName)
			{
				bInTHRotatorSection = true;
			}

			continue;
		}

		std::smatch match;
		if (!std::regex_match(line, match, keyValuePattern))
		{
			if (std::regex_match(line, match, sectionPattern))
			{
				break;
			}
		}

		const auto& key = match.str(1);
		const auto& value = match.str(2);

		valueMap[key] = value;
	}

	ReadIniValue(valueMap, "JC", outSetting.judgeThreshold);
	ReadIniValue(valueMap, "PL", outSetting.mainScreenTopLeft.x);
	ReadIniValue(valueMap, "PT", outSetting.mainScreenTopLeft.y);
	ReadIniValue(valueMap, "PW", outSetting.mainScreenSize.cx);
	ReadIniValue(valueMap, "PH", outSetting.mainScreenSize.cy);
	ReadIniValue(valueMap, "YOffset", outSetting.yOffset);

	BOOL bVisibleTemp = FALSE;
	ReadIniValue(valueMap, "Visible", bVisibleTemp);
	outSetting.bVisible = bVisibleTemp != FALSE;

	BOOL bVerticallyLongWindowTemp = FALSE;
	ReadIniValue(valueMap, "PivRot", bVerticallyLongWindowTemp);
	outSetting.bVerticallyLongWindow = bVerticallyLongWindowTemp != FALSE;

	ReadIniValue(valueMap, "Rot", outSetting.rotationAngle);
	ReadIniValue(valueMap, "Filter", outSetting.filterType);

	outSetting.rectTransfers.clear();

	BOOL bHasNext = FALSE;
	ReadIniValue(valueMap, "ORHas0", bHasNext);
	int rectIndex = 0;
	while (bHasNext)
	{
		RectTransferData rectData;

		std::string rectName;
		ReadIndexedIniValue(valueMap, "Name", rectIndex, rectName);

		rectData.name = ConvertFromSjisToUtf8(rectName);

		ReadIndexedIniValue(valueMap, "OSL", rectIndex, rectData.sourcePosition.x);
		ReadIndexedIniValue(valueMap, "OST", rectIndex, rectData.sourcePosition.y);
		ReadIndexedIniValue(valueMap, "OSW", rectIndex, rectData.sourceSize.cx);
		ReadIndexedIniValue(valueMap, "OSH", rectIndex, rectData.sourceSize.cy);

		ReadIndexedIniValue(valueMap, "ODL", rectIndex, rectData.destPosition.x);
		ReadIndexedIniValue(valueMap, "ODT", rectIndex, rectData.destPosition.y);
		ReadIndexedIniValue(valueMap, "ODW", rectIndex, rectData.destSize.cx);
		ReadIndexedIniValue(valueMap, "ODH", rectIndex, rectData.destSize.cy);

		ReadIndexedIniValue(valueMap, "OR", rectIndex, rectData.rotation);

		outSetting.rectTransfers.push_back(rectData);
		rectIndex++;

		ReadIndexedIniValue(valueMap, "ORHas", rectIndex, bHasNext);
	}
}

void THRotatorSetting::LoadJsonFormat(THRotatorSetting& outSetting,
	THRotatorFormatVersion& importedFormatVersion)
{
	auto filename = CreateJsonFilePath();

	OutputLogMessagef(LogSeverity::Info, "Loading {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));

	std::ifstream ifs;
	ifs.open(filename.generic_wstring());

	// json.hpp doesn't allow ifstream's exception bit.
	if (ifs.fail() || ifs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to load {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));
		throw std::ios::failure("");
	}

	nlohmann::json loadedJson;

	ifs >> loadedJson;

	ReadJsonObjectValue(loadedJson, "format_version", importedFormatVersion);

	outSetting = loadedJson;
}

bool THRotatorSetting::Load(THRotatorSetting& outSetting,
	THRotatorFormatVersion& importedFormatVersion)
{
	try
	{
		THRotatorSetting::LoadJsonFormat(outSetting, importedFormatVersion);
	}
	catch (const std::ios::failure&)
	{
		OutputLogMessagef(LogSeverity::Info, "Falling back to legacy format v1 loading");
		THRotatorSetting::LoadIniFormat(outSetting);
		importedFormatVersion = THRotatorFormatVersion::Version_1;
	}
	catch (const std::exception& e)
	{
		OutputLogMessagef(LogSeverity::Error, "Json parse failed ({0})", e.what());
		return false;
	}

	return true;
}

bool THRotatorSetting::Save(const THRotatorSetting& inSetting)
{
	nlohmann::json objectToWrite(nlohmann::json::value_t::object);

	objectToWrite = inSetting;

	auto filename = CreateJsonFilePath();
	OutputLogMessagef(LogSeverity::Info, "Saving to {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));
	std::ofstream ofs(filename.generic_wstring());

	if (ofs.fail() || ofs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to write to {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));
		return false;
	}

	ofs << objectToWrite.dump(2);

	return true;
}
