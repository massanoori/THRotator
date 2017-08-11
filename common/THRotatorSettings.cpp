// (c) 2017 massanoori

#include "stdafx.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/categories.hpp> // input_filter_tag
#include <boost/iostreams/operations.hpp> // get, WOULD_BLOCK
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include "THRotatorSettings.h"
#include "EncodingUtils.h"

namespace
{

const int UTF8_BOM[3] =
{
	0xEF, 0xBB, 0xBF,
};

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

struct InputFilerIgnoringBOM
{
	typedef char char_type;
	typedef boost::iostreams::input_filter_tag category;

	bool bBOMRead;
	std::list<int> residualsOfBOMSkip;

	InputFilerIgnoringBOM() : bBOMRead(false)
	{
	}

	template<typename Source>
	int get(Source& src)
	{
		if (!bBOMRead)
		{
			int readBOM[3];

			readBOM[0] = boost::iostreams::get(src);
			readBOM[1] = boost::iostreams::get(src);
			readBOM[2] = boost::iostreams::get(src);

			if (readBOM[0] != UTF8_BOM[0] ||
				readBOM[1] != UTF8_BOM[1] ||
				readBOM[2] != UTF8_BOM[2])
			{
				residualsOfBOMSkip.push_back(readBOM[0]);
				residualsOfBOMSkip.push_back(readBOM[1]);
				residualsOfBOMSkip.push_back(readBOM[2]);
			}

			bBOMRead = true;
		}

		if (!residualsOfBOMSkip.empty())
		{
			int residual = residualsOfBOMSkip.front();
			residualsOfBOMSkip.pop_front();
			return residual;
		}

		return boost::iostreams::get(src);
	}
};

struct OutputFilterWithBOM
{
	typedef char char_type;
	typedef boost::iostreams::output_filter_tag category;

	bool bBOMWritten;

	OutputFilterWithBOM() : bBOMWritten(false)
	{
	}

	template <typename Sink>
	bool put(Sink& sink, char_type c)
	{
		if (!bBOMWritten)
		{
			boost::iostreams::put(sink, UTF8_BOM[0]);
			boost::iostreams::put(sink, UTF8_BOM[1]);
			boost::iostreams::put(sink, UTF8_BOM[2]);

			bBOMWritten = true;
		}

		return boost::iostreams::put(sink, c);
	}
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
		boost::iostreams::filtering_istream inStream;
		inStream.push(InputFilerIgnoringBOM());
		inStream.push(boost::iostreams::file_descriptor_source(filename));
		proptree::read_ini(inStream, tree);
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

	namespace proptree = boost::property_tree;

	proptree::basic_ptree<std::string, std::string> tree;
	try
	{
		boost::iostreams::filtering_istream inStream;
		inStream.push(InputFilerIgnoringBOM());
		inStream.push(boost::iostreams::file_descriptor_source(filename));
		proptree::read_json(inStream, tree);
	}
	catch (const proptree::ini_parser_error&)
	{
		// デフォルト値を用いる
		return;
	}

#define READ_JSON_PARAM(parent, destination, name) \
	do { \
		auto rawValue = parent.get_optional<RawTypeOfEnum<decltype(destination)>::type>(name); \
		if (rawValue) (destination) = static_cast<std::remove_reference<decltype(destination)>::type>(*rawValue); \
	} while(false)

	READ_JSON_PARAM(tree, outSetting.judgeThreshold, "judge_threshold");
	READ_JSON_PARAM(tree, outSetting.mainScreenTopLeft.x, "main_screen_left");
	READ_JSON_PARAM(tree, outSetting.mainScreenTopLeft.y, "main_screen_top");
	READ_JSON_PARAM(tree, outSetting.mainScreenSize.cx, "main_screen_width");
	READ_JSON_PARAM(tree, outSetting.mainScreenSize.cy, "main_screen_height");
	READ_JSON_PARAM(tree, outSetting.yOffset, "y_offset");
	READ_JSON_PARAM(tree, outSetting.bVisible, "window_visible");
	READ_JSON_PARAM(tree, outSetting.bVerticallyLongWindow, "vertical_window");
	READ_JSON_PARAM(tree, outSetting.bModalEditorPreferred, "use_modal_editor");
	READ_JSON_PARAM(tree, outSetting.rotationAngle, "rotation_angle");
	READ_JSON_PARAM(tree, outSetting.filterType, "fileter_type");
	READ_JSON_PARAM(tree, importedFormatVersion, "format_version");

	const auto& loadedRectTransfersOptional = tree.get_child_optional("rects");
	if (!loadedRectTransfersOptional)
	{
		// 矩形転送なし
		return;
	}

	const auto& loadedRectTransfers = *loadedRectTransfersOptional;

	std::vector<RectTransferData> newRectTransfers;
	newRectTransfers.reserve(loadedRectTransfers.size());

	for (const auto& loadedRectTransfer : loadedRectTransfers)
	{
		const auto& rectTransferNode = loadedRectTransfer.second;

		RectTransferData rectData;
		std::string rectName;
		READ_JSON_PARAM(rectTransferNode, rectName, "rect_name");

#ifdef _UNICODE
		rectData.name = ConvertFromUtf8ToUnicode(rectName);
#else
		rectData.name = ConvertFromUtf8ToSjis(rectName);
#endif

		READ_JSON_PARAM(rectTransferNode, rectData.sourcePosition.x, "source_left");
		READ_JSON_PARAM(rectTransferNode, rectData.sourcePosition.y, "source_top");
		READ_JSON_PARAM(rectTransferNode, rectData.sourceSize.cx, "source_width");
		READ_JSON_PARAM(rectTransferNode, rectData.sourceSize.cy, "source_height");

		READ_JSON_PARAM(rectTransferNode, rectData.destPosition.x, "destination_left");
		READ_JSON_PARAM(rectTransferNode, rectData.destPosition.y, "destination_top");
		READ_JSON_PARAM(rectTransferNode, rectData.destSize.cx, "destination_width");
		READ_JSON_PARAM(rectTransferNode, rectData.destSize.cy, "destination_height");

		READ_JSON_PARAM(rectTransferNode, rectData.rotation, "rect_rotation");

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
	namespace proptree = boost::property_tree;
	proptree::basic_ptree<std::string, std::string> tree;

#define WRITE_JSON_PARAM(parent, name, value) parent.add(name, static_cast<RawTypeOfEnum<decltype(value)>::type>(value))

	WRITE_JSON_PARAM(tree, "format_version", THRotatorFormatVersion::Latest);
	WRITE_JSON_PARAM(tree, "judge_threshold", inSetting.judgeThreshold);
	WRITE_JSON_PARAM(tree, "main_screen_left", inSetting.mainScreenTopLeft.x);
	WRITE_JSON_PARAM(tree, "main_screen_top", inSetting.mainScreenTopLeft.y);
	WRITE_JSON_PARAM(tree, "main_screen_width", inSetting.mainScreenSize.cx);
	WRITE_JSON_PARAM(tree, "main_screen_height", inSetting.mainScreenSize.cy);
	WRITE_JSON_PARAM(tree, "y_offset", inSetting.yOffset);
	WRITE_JSON_PARAM(tree, "window_visible", inSetting.bVisible);
	WRITE_JSON_PARAM(tree, "vertical_window", inSetting.bVerticallyLongWindow);
	WRITE_JSON_PARAM(tree, "fileter_type", inSetting.filterType);
	WRITE_JSON_PARAM(tree, "rotation_angle", inSetting.rotationAngle);
	WRITE_JSON_PARAM(tree, "NumRectTransfers", inSetting.rectTransfers.size());
	WRITE_JSON_PARAM(tree, "use_modal_editor", inSetting.bModalEditorPreferred);

	proptree::basic_ptree<std::string, std::string> rectTransfersNode;

	for (const auto& rectData : inSetting.rectTransfers)
	{
#ifdef _UNICODE
		const auto& nameToSave = ConvertFromUnicodeToUtf8(rectData.name);
#else
		const auto& nameToSave = ConvertFromSjisToUtf8(rectData.name);
#endif

		proptree::basic_ptree<std::string, std::string> rectTransferNode;

		WRITE_JSON_PARAM(rectTransferNode, "rect_name", nameToSave);

		WRITE_JSON_PARAM(rectTransferNode, "source_left", rectData.sourcePosition.x);
		WRITE_JSON_PARAM(rectTransferNode, "source_top", rectData.sourcePosition.y);
		WRITE_JSON_PARAM(rectTransferNode, "source_width", rectData.sourceSize.cx);
		WRITE_JSON_PARAM(rectTransferNode, "source_height", rectData.sourceSize.cy);

		WRITE_JSON_PARAM(rectTransferNode, "destination_left", rectData.destPosition.x);
		WRITE_JSON_PARAM(rectTransferNode, "destination_top", rectData.destPosition.y);
		WRITE_JSON_PARAM(rectTransferNode, "destination_width", rectData.destSize.cx);
		WRITE_JSON_PARAM(rectTransferNode, "destination_height", rectData.destSize.cy);

		WRITE_JSON_PARAM(rectTransferNode, "rect_rotation", rectData.rotation);

		rectTransfersNode.push_back(std::make_pair("", rectTransferNode));
	}

	tree.add_child("rects", rectTransfersNode);

#undef WRITE_INDEXED_INI_PARAM
#undef WRITE_INI_PARAM

	auto filename = CreateJsonFilePath(processWorkingDir, exeFilename);

	try
	{
		boost::iostreams::filtering_ostream outStream;
		outStream.push(OutputFilterWithBOM());
		outStream.push(boost::iostreams::file_descriptor_sink(filename));

		proptree::write_json(outStream, tree);
	}
	catch (const std::ios::failure&)
	{
		return false;
	}

	return true;
}
