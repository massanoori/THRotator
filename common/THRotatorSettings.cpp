// (c) 2017 massanoori

#include "stdafx.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <json.hpp>

#include "THRotatorSettings.h"
#include "EncodingUtils.h"

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
struct JsonConvHelper
{

};

template <>
struct JsonConvHelper<int>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_number_integer();
	}

	static int ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		int i = j;
		return i;
	}

	static nlohmann::json ConvertToJson(int x)
	{
		return nlohmann::json(x);
	}
};

template <>
struct JsonConvHelper<long>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_number_integer();
	}

	static long ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		int i = j;
		return i;
	}

	static nlohmann::json ConvertToJson(long x)
	{
		return nlohmann::json(x);
	}
};

template <>
struct JsonConvHelper<unsigned int>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_number_integer();
	}

	static unsigned int ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		unsigned int i = j;
		return i;
	}

	static nlohmann::json ConvertToJson(unsigned int x)
	{
		return nlohmann::json(x);
	}
};

template <>
struct JsonConvHelper<RotationAngle>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_number_integer();
	}

	static RotationAngle ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		typename std::underlying_type<RotationAngle>::type i = j;
		return static_cast<RotationAngle>(i);
	}

	static nlohmann::json ConvertToJson(RotationAngle x)
	{
		return nlohmann::json(static_cast<std::underlying_type<RotationAngle>::type>(x));
	}
};

template <>
struct JsonConvHelper<THRotatorFormatVersion>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_number_integer();
	}

	static THRotatorFormatVersion ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		typename std::underlying_type<THRotatorFormatVersion>::type i = j;
		return static_cast<THRotatorFormatVersion>(i);
	}

	static nlohmann::json ConvertToJson(THRotatorFormatVersion x)
	{
		return nlohmann::json(static_cast<std::underlying_type<THRotatorFormatVersion>::type>(x));
	}
};

template <>
struct JsonConvHelper<bool>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_boolean();
	}

	static bool ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		bool b = j;
		return b;
	}

	static nlohmann::json ConvertToJson(bool x)
	{
		return nlohmann::json(x);
	}
};

template <>
struct JsonConvHelper<D3DTEXTUREFILTERTYPE>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		if (!j.is_string())
		{
			return false;
		}

		std::string value = j;
		return value == "linear" || value == "none";
	}

	static D3DTEXTUREFILTERTYPE ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		std::string value = j;

		if (value == "linear")
		{
			return D3DTEXF_LINEAR;
		}
		else
		{
			return D3DTEXF_NONE;
		}
	}

	static nlohmann::json ConvertToJson(D3DTEXTUREFILTERTYPE x)
	{
		switch (x)
		{
		case D3DTEXF_NONE:
			return nlohmann::json("none");

		case D3DTEXF_LINEAR:
			return nlohmann::json("linear");

		default:
			break;
		}

		return nlohmann::json("linear");
	}
};

template <>
struct JsonConvHelper<std::string>
{
	static bool IsValidType(const nlohmann::json& j)
	{
		return j.is_string();
	}

	static std::string ConvertFromJson(const nlohmann::json& j)
	{
		assert(IsValidType(j));
		std::string s = j;
		return s;
	}

	static nlohmann::json ConvertToJson(const std::string& x)
	{
		return nlohmann::json(x);
	}
};

template <typename T>
struct JsonConv : public JsonConvHelper<typename std::remove_const<typename std::remove_reference<T>::type>::type>
{
};

}

void THRotatorSetting::LoadIniFormat(const boost::filesystem::path& processWorkingDir,
	const boost::filesystem::path& exeFilename,
	THRotatorSetting& outSetting)
{
	namespace proptree = boost::property_tree;

	auto filename = CreateIniFilePath(processWorkingDir);

	proptree::basic_ptree<std::string, std::string> tree;
	try
	{
		proptree::read_ini(filename.generic_string(), tree);
	}
	catch (const std::ios::failure&)
	{
		// デフォルト値を用いる
		return;
	}
	catch (const proptree::ini_parser_error&)
	{
		// デフォルト値を用いる
		return;
	}

	auto appName = GenerateIniAppName(exeFilename); // READ_INI_PARAMの中で参照

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

void THRotatorSetting::LoadJsonFormat(const boost::filesystem::path& processWorkingDir,
	const boost::filesystem::path& exeFilename,
	THRotatorSetting& outSetting,
	THRotatorFormatVersion& importedFormatVersion)
{
	auto filename = CreateJsonFilePath(processWorkingDir, exeFilename);

	std::ifstream ifs;
	ifs.open(filename.generic_wstring());

	// json.hppでは、ifstreamの例外ビットを設定することができない
	if (ifs.fail() || ifs.bad())
	{
		throw std::ios::failure("");
	}

	nlohmann::json loadedJson;

	try
	{
		ifs >> loadedJson;
	}
	catch (const nlohmann::json::parse_error&)
	{
		// デフォルト値を用いる
		return;
	}

#define READ_JSON_PARAM(parent, destination, name) \
	do { \
		auto foundItr = parent.find(name); \
		if (foundItr != parent.end() && JsonConv<typename std::remove_reference<decltype(destination)>::type>::IsValidType(*foundItr)) \
		{ \
			(destination) = JsonConv<typename std::remove_reference<decltype(destination)>::type>::ConvertFromJson(*foundItr); \
		} \
	} while(false)

	READ_JSON_PARAM(loadedJson, outSetting.judgeThreshold, "judge_threshold");
	READ_JSON_PARAM(loadedJson, outSetting.mainScreenTopLeft.x, "main_screen_left");
	READ_JSON_PARAM(loadedJson, outSetting.mainScreenTopLeft.y, "main_screen_top");
	READ_JSON_PARAM(loadedJson, outSetting.mainScreenSize.cx, "main_screen_width");
	READ_JSON_PARAM(loadedJson, outSetting.mainScreenSize.cy, "main_screen_height");
	READ_JSON_PARAM(loadedJson, outSetting.yOffset, "y_offset");
	READ_JSON_PARAM(loadedJson, outSetting.bVisible, "window_visible");
	READ_JSON_PARAM(loadedJson, outSetting.bVerticallyLongWindow, "vertical_window");
	READ_JSON_PARAM(loadedJson, outSetting.bModalEditorPreferred, "use_modal_editor");
	READ_JSON_PARAM(loadedJson, outSetting.rotationAngle, "rotation_angle");
	READ_JSON_PARAM(loadedJson, outSetting.filterType, "fileter_type");
	READ_JSON_PARAM(loadedJson, importedFormatVersion, "format_version");

	auto rectsItr = loadedJson.find("rects");
	if (rectsItr == loadedJson.end() && rectsItr->is_array())
	{
		outSetting.rectTransfers.clear();
	}

	auto rectsObject = *rectsItr;

	std::vector<RectTransferData> newRectTransfers;
	newRectTransfers.reserve(rectsObject.size());

	for (const auto& rectObject : rectsObject)
	{
		RectTransferData rectData;
		std::string rectName;
		READ_JSON_PARAM(rectObject, rectName, "rect_name");

#ifdef _UNICODE
		rectData.name = ConvertFromUtf8ToUnicode(rectName);
#else
		rectData.name = ConvertFromUtf8ToSjis(rectName);
#endif

		READ_JSON_PARAM(rectObject, rectData.sourcePosition.x, "source_left");
		READ_JSON_PARAM(rectObject, rectData.sourcePosition.y, "source_top");
		READ_JSON_PARAM(rectObject, rectData.sourceSize.cx, "source_width");
		READ_JSON_PARAM(rectObject, rectData.sourceSize.cy, "source_height");

		READ_JSON_PARAM(rectObject, rectData.destPosition.x, "destination_left");
		READ_JSON_PARAM(rectObject, rectData.destPosition.y, "destination_top");
		READ_JSON_PARAM(rectObject, rectData.destSize.cx, "destination_width");
		READ_JSON_PARAM(rectObject, rectData.destSize.cy, "destination_height");

		READ_JSON_PARAM(rectObject, rectData.rotation, "rect_rotation");

		newRectTransfers.push_back(rectData);
	}

	outSetting.rectTransfers = std::move(newRectTransfers);

#undef READ_JSON_PARAM
}

std::string THRotatorSetting::GenerateIniAppName(const boost::filesystem::path& exeFilename)
{
	return ConvertFromUnicodeToSjis(std::wstring(L"THRotator_") + exeFilename.stem().generic_wstring());
}

boost::filesystem::path THRotatorSetting::CreateIniFilePath(const boost::filesystem::path& processWorkingDir)
{
	return processWorkingDir / L"throt.ini";
}

boost::filesystem::path THRotatorSetting::CreateJsonFilePath(const boost::filesystem::path& processWorkingDir, const boost::filesystem::path& exeFilename)
{
	auto jsonPath = processWorkingDir / exeFilename;
	jsonPath += L".throtator";
	return jsonPath;
}

void THRotatorSetting::Load(const boost::filesystem::path& processWorkingDir,
	const boost::filesystem::path& exeFilename,
	THRotatorSetting& outSetting,
	THRotatorFormatVersion& importedFormatVersion)
{
	try
	{
		THRotatorSetting::LoadJsonFormat(processWorkingDir, exeFilename, outSetting, importedFormatVersion);
	}
	catch (const std::ios::failure&)
	{
		THRotatorSetting::LoadIniFormat(processWorkingDir, exeFilename, outSetting);
		importedFormatVersion = THRotatorFormatVersion::Version_1;
	}
}

bool THRotatorSetting::Save(const boost::filesystem::path& processWorkingDir, const boost::filesystem::path& exeFilename, const THRotatorSetting& inSetting)
{
	nlohmann::json objectToWrite;

#define WRITE_JSON_PARAM(parent, name, value) parent[name] = JsonConv<decltype(value)>::ConvertToJson(value)

	WRITE_JSON_PARAM(objectToWrite, "format_version", THRotatorFormatVersion::Latest);
	WRITE_JSON_PARAM(objectToWrite, "judge_threshold", inSetting.judgeThreshold);
	WRITE_JSON_PARAM(objectToWrite, "main_screen_left", inSetting.mainScreenTopLeft.x);
	WRITE_JSON_PARAM(objectToWrite, "main_screen_top", inSetting.mainScreenTopLeft.y);
	WRITE_JSON_PARAM(objectToWrite, "main_screen_width", inSetting.mainScreenSize.cx);
	WRITE_JSON_PARAM(objectToWrite, "main_screen_height", inSetting.mainScreenSize.cy);
	WRITE_JSON_PARAM(objectToWrite, "y_offset", inSetting.yOffset);
	WRITE_JSON_PARAM(objectToWrite, "window_visible", inSetting.bVisible);
	WRITE_JSON_PARAM(objectToWrite, "vertical_window", inSetting.bVerticallyLongWindow);
	WRITE_JSON_PARAM(objectToWrite, "fileter_type", inSetting.filterType);
	WRITE_JSON_PARAM(objectToWrite, "rotation_angle", inSetting.rotationAngle);
	WRITE_JSON_PARAM(objectToWrite, "NumRectTransfers", inSetting.rectTransfers.size());
	WRITE_JSON_PARAM(objectToWrite, "use_modal_editor", inSetting.bModalEditorPreferred);

	nlohmann::json rectsArray;

	for (const auto& rectData : inSetting.rectTransfers)
	{
		nlohmann::json rectObject;

#ifdef _UNICODE
		const auto& nameToSave = ConvertFromUnicodeToUtf8(rectData.name);
#else
		const auto& nameToSave = ConvertFromSjisToUtf8(rectData.name);
#endif

		WRITE_JSON_PARAM(rectObject, "rect_name", nameToSave);

		WRITE_JSON_PARAM(rectObject, "source_left", rectData.sourcePosition.x);
		WRITE_JSON_PARAM(rectObject, "source_top", rectData.sourcePosition.y);
		WRITE_JSON_PARAM(rectObject, "source_width", rectData.sourceSize.cx);
		WRITE_JSON_PARAM(rectObject, "source_height", rectData.sourceSize.cy);

		WRITE_JSON_PARAM(rectObject, "destination_left", rectData.destPosition.x);
		WRITE_JSON_PARAM(rectObject, "destination_top", rectData.destPosition.y);
		WRITE_JSON_PARAM(rectObject, "destination_width", rectData.destSize.cx);
		WRITE_JSON_PARAM(rectObject, "destination_height", rectData.destSize.cy);

		WRITE_JSON_PARAM(rectObject, "rect_rotation", rectData.rotation);

		rectsArray.push_back(rectObject);
	}

	objectToWrite["rects"] = rectsArray;

#undef WRITE_INI_PARAM

	auto filename = CreateJsonFilePath(processWorkingDir, exeFilename);
	std::ofstream ofs(filename.generic_wstring());

	if (ofs.fail() || ofs.bad())
	{
		return false;
	}

	ofs << objectToWrite.dump(2);

	return true;
}
