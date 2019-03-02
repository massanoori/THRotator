// (c) 2017 massanoori

#include "stdafx.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <nlohmann/json.hpp>

#include "THRotatorSettings.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"

namespace
{

// Template for retrieving underlying type of enum of T if T is enum, otherwise T.
template <typename T, typename IsEnum = typename std::is_enum<typename std::remove_reference<T>::type>::type>
struct RawTypeOfEnum
{
	typedef typename std::remove_reference<T>::type type;
};

// Specialization for T of enum.
template <typename T>
struct RawTypeOfEnum<T, std::true_type>
{
	typedef typename std::underlying_type<typename std::remove_reference<T>::type>::type type;
};

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
	namespace proptree = boost::property_tree;

	auto filename = CreateIniFilePath();
	OutputLogMessagef(LogSeverity::Info, "Loading {0}", ConvertFromUnicodeToUtf8(filename.generic_wstring()));

	proptree::basic_ptree<std::string, std::string> tree;
	try
	{
		proptree::read_ini(filename.generic_string(), tree);
	}
	catch (const proptree::ini_parser_error& e)
	{
		OutputLogMessagef(LogSeverity::Error, "Ini parse error ({0})", e.message());
		return;
	}

	auto appName = GenerateIniAppName(); // Referenced by READ_INI_PARAM macro

#define READ_INI_PARAM(destination, name) \
	do { \
		auto rawValue = tree.get_optional<RawTypeOfEnum<decltype(destination)>::type>(appName + "." + name); \
		if (rawValue) (destination) = static_cast<decltype(destination)>(*rawValue); \
	} while(false)

#define READ_INDEXED_INI_PARAM(destination, name, index) \
	do { \
		std::ostringstream ss(std::ios::ate); \
		ss.str(name); \
		ss << (index); \
		READ_INI_PARAM(destination, ss.str()); \
	} while(false)

	READ_INI_PARAM(outSetting.judgeThreshold, "JC");
	READ_INI_PARAM(outSetting.mainScreenTopLeft.x, "PL");
	READ_INI_PARAM(outSetting.mainScreenTopLeft.y, "PT");
	READ_INI_PARAM(outSetting.mainScreenSize.cx, "PW");
	READ_INI_PARAM(outSetting.mainScreenSize.cy, "PH");
	READ_INI_PARAM(outSetting.yOffset, "YOffset");

	BOOL bVisibleTemp = FALSE;
	READ_INI_PARAM(bVisibleTemp, "Visible");
	outSetting.bVisible = bVisibleTemp != FALSE;

	BOOL bVerticallyLongWindowTemp = FALSE;
	READ_INI_PARAM(bVerticallyLongWindowTemp, "PivRot");
	outSetting.bVerticallyLongWindow = bVerticallyLongWindowTemp != FALSE;

	READ_INI_PARAM(outSetting.rotationAngle, "Rot");
	READ_INI_PARAM(outSetting.filterType, "Filter");

	outSetting.rectTransfers.clear();

	BOOL bHasNext = FALSE;
	READ_INI_PARAM(bHasNext, "ORHas0");
	int rectIndex = 0;
	while (bHasNext)
	{
		RectTransferData rectData;

		std::string rectName;
		READ_INDEXED_INI_PARAM(rectName, "Name", rectIndex);

		rectData.name = ConvertFromSjisToUtf8(rectName);

		READ_INDEXED_INI_PARAM(rectData.sourcePosition.x, "OSL", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.sourcePosition.y, "OST", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.sourceSize.cx, "OSW", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.sourceSize.cy, "OSH", rectIndex);

		READ_INDEXED_INI_PARAM(rectData.destPosition.x, "ODL", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.destPosition.y, "ODT", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.destSize.cx, "ODW", rectIndex);
		READ_INDEXED_INI_PARAM(rectData.destSize.cy, "ODH", rectIndex);

		READ_INDEXED_INI_PARAM(rectData.rotation, "OR", rectIndex);

		outSetting.rectTransfers.push_back(rectData);
		rectIndex++;

		READ_INDEXED_INI_PARAM(bHasNext, "ORHas", rectIndex);
	}

#undef READ_INDEXED_INI_PARAM
#undef READ_INI_PARAM
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
