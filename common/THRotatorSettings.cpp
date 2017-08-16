// (c) 2017 massanoori

#include "stdafx.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// Windows.hで定義されるmin、maxマクロをundefし、
// 外部ライブラリが使う、std名前空間以下のmin, max関数が使えなくなるのを防ぐ
#undef min
#undef max

#include <json.hpp>

#include "THRotatorSettings.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"

namespace
{

// Tがenumなら、その内部の型を、そうでなければTを、RawTypeOfEnum<T>::typeで取得できる
template <typename T, typename IsEnum = typename std::is_enum<typename std::remove_reference<T>::type>::type>
struct RawTypeOfEnum
{
	typedef typename std::remove_reference<T>::type type;
};

// Tがenumの時の特殊化
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

template <typename T>
void ReadJsonObjectValueKeepOnFailure(const nlohmann::json& j, const std::string& key, T& out)
{
	try
	{
		ReadJsonObjectValue(j, key, out);
	}
	catch (const std::logic_error& e)
	{
		OutputLogMessage(LogSeverity::Error, ConvertFromSjisToUnicode(e.what()));
	}
}

std::string GenerateIniAppName()
{
	return ConvertFromUnicodeToSjis(std::wstring(L"THRotator_") + GetTouhouExecutableFilename().stem().generic_wstring());
}

boost::filesystem::path CreateIniFilePath()
{
	return GetTouhouPlayerDataDirectory() / L"throt.ini";
}

boost::filesystem::path CreateJsonFilePath()
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

#ifdef _UNICODE
	rectData.name = ConvertFromUtf8ToUnicode(rectName);
#else
	rectData.name = ConvertFromUtf8ToSjis(rectName);
#endif

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
#ifdef _UNICODE
	std::string rectName = ConvertFromUnicodeToUtf8(rectData.name);
#else
	std::string rectName = ConvertFromSjisToUtf8(rectData.name);
#endif
	j["rect_name"] = rectName;

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
		OutputLogMessagef(LogSeverity::Error, L"type must be object, but is {0}", j.type_name());
		return;
	}

	// used for migration
	THRotatorFormatVersion formatVersion;
	ReadJsonObjectValueKeepOnFailure(j, "format_version", formatVersion);

	ReadJsonObjectValueKeepOnFailure(j, "judge_threshold", setting.judgeThreshold);
	ReadJsonObjectValueKeepOnFailure(j, "main_screen_left", setting.mainScreenTopLeft.x);
	ReadJsonObjectValueKeepOnFailure(j, "main_screen_top", setting.mainScreenTopLeft.y);
	ReadJsonObjectValueKeepOnFailure(j, "main_screen_width", setting.mainScreenSize.cx);
	ReadJsonObjectValueKeepOnFailure(j, "main_screen_height", setting.mainScreenSize.cy);
	ReadJsonObjectValueKeepOnFailure(j, "y_offset", setting.yOffset);
	ReadJsonObjectValueKeepOnFailure(j, "window_visible", setting.bVisible);
	ReadJsonObjectValueKeepOnFailure(j, "vertical_window", setting.bVerticallyLongWindow);
	ReadJsonObjectValueKeepOnFailure(j, "use_modal_editor", setting.bModalEditorPreferred);
	ReadJsonObjectValueKeepOnFailure(j, "rotation_angle", setting.rotationAngle);
	ReadJsonObjectValueKeepOnFailure(j, "fileter_type", setting.filterType);

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
		try
		{
			RectTransferData rectData = rectObject;
			setting.rectTransfers.push_back(rectData);
		}
		catch (const std::logic_error& e)
		{
			OutputLogMessagef(LogSeverity::Error, L"Failed to load 'rects[{0}]' ({1})", currentRectIndex, e.what());
		}

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
	j["use_modal_editor"] = setting.bModalEditorPreferred;

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
	OutputLogMessagef(LogSeverity::Info, L"Loading {0}", filename.generic_wstring());

	proptree::basic_ptree<std::string, std::string> tree;
	try
	{
		proptree::read_ini(filename.generic_string(), tree);
	}
	catch (const proptree::ini_parser_error& e)
	{
		OutputLogMessagef(LogSeverity::Error, L"Ini parse error ({0})", ConvertFromSjisToUnicode(e.message()));
		return;
	}

	auto appName = GenerateIniAppName(); // READ_INI_PARAMの中で参照

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

#ifdef _UNICODE
		rectData.name = ConvertFromSjisToUnicode(rectName);
#else
		rectData.name = std::move(rectName);
#endif

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

	OutputLogMessagef(LogSeverity::Info, L"Loading {0}", filename.generic_wstring());

	std::ifstream ifs;
	ifs.open(filename.generic_wstring());

	// json.hppでは、ifstreamの例外ビットを設定することができない
	if (ifs.fail() || ifs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, L"Failed to load {0}", filename.generic_wstring());
		throw std::ios::failure("");
	}

	nlohmann::json loadedJson;

	ifs >> loadedJson;

	ReadJsonObjectValueKeepOnFailure(loadedJson, "format_version", importedFormatVersion);

	outSetting = loadedJson;
}

bool THRotatorSetting::Load(THRotatorSetting& outSetting,
	THRotatorFormatVersion& importedFormatVersion)
{
	try
	{
		THRotatorSetting::LoadJsonFormat(outSetting, importedFormatVersion);
	}
	catch (const std::invalid_argument& e)
	{
		OutputLogMessagef(LogSeverity::Error, L"Json parse failed ({0})", ConvertFromSjisToUnicode(e.what()));
		return false;
	}
	catch (const std::ios::failure&)
	{
		OutputLogMessagef(LogSeverity::Info, L"Falling back to legacy format v1 loading");
		THRotatorSetting::LoadIniFormat(outSetting);
		importedFormatVersion = THRotatorFormatVersion::Version_1;
	}

	return true;
}

bool THRotatorSetting::Save(const THRotatorSetting& inSetting)
{
	nlohmann::json objectToWrite(nlohmann::json::value_t::object);

	objectToWrite = inSetting;

	auto filename = CreateJsonFilePath();
	OutputLogMessagef(LogSeverity::Info, L"Saving to {0}", filename.generic_wstring());
	std::ofstream ofs(filename.generic_wstring());

	if (ofs.fail() || ofs.bad())
	{
		OutputLogMessagef(LogSeverity::Error, L"Failed to write to {0}", filename.generic_wstring());
		return false;
	}

	ofs << objectToWrite.dump(2);

	return true;
}
