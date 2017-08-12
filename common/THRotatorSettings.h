// (c) 2017 massanoori

#pragma once

enum RotationAngle : std::uint32_t
{
	Rotation_0 = 0,
	Rotation_90 = 1,
	Rotation_180 = 2,
	Rotation_270 = 3,

	Rotation_Num,
};

enum class THRotatorFormatVersion
{
	Version_1 = 1,
	Version_2,

	LatestPlusOne,
	Latest = LatestPlusOne - 1,
};

struct RectTransferData
{
	POINT sourcePosition;
	SIZE sourceSize;

	POINT destPosition;
	SIZE destSize;

	RotationAngle rotation;
	std::basic_string<TCHAR> name;

	RectTransferData()
		: rotation(Rotation_0)
	{
		sourcePosition.x = 0;
		sourcePosition.y = 0;
		sourceSize.cx = 0;
		sourceSize.cy = 0;
		destPosition.x = 0;
		destPosition.y = 0;
		destSize.cx = 0;
		destSize.cy = 0;
	}
};

struct THRotatorSetting
{
	int judgeThreshold;
	POINT mainScreenTopLeft;
	SIZE mainScreenSize;
	int yOffset;
	bool bVisible;
	bool bVerticallyLongWindow;
	bool bModalEditorPreferred;
	RotationAngle rotationAngle;
	D3DTEXTUREFILTERTYPE filterType;

	std::vector<RectTransferData> rectTransfers;

	THRotatorSetting()
		: judgeThreshold(999)
		, yOffset(0)
		, bVisible(false)
		, bVerticallyLongWindow(false)
		, bModalEditorPreferred(false)
		, rotationAngle(Rotation_0)
		, filterType(D3DTEXF_LINEAR)
	{
		mainScreenTopLeft.x = 32;
		mainScreenTopLeft.y = 16;
		mainScreenSize.cx = 384;
		mainScreenSize.cy = 448;
	}

	static bool Load(const boost::filesystem::path& processWorkingDir,
		const boost::filesystem::path& exeFilename,
		THRotatorSetting& outSetting,
		THRotatorFormatVersion& importedFormatVersion,
		std::list<std::wstring>& outMessages);

	static bool Save(const boost::filesystem::path& processWorkingDir,
		const boost::filesystem::path& exeFilename,
		const THRotatorSetting& inSetting,
		std::list<std::wstring>& outMessages);

	static void LoadIniFormat(const boost::filesystem::path& processWorkingDir,
		const boost::filesystem::path& exeFilename,
		THRotatorSetting& outSetting,
		std::list<std::wstring>& outMessages);

	static void LoadJsonFormat(const boost::filesystem::path& processWorkingDir,
		const boost::filesystem::path& exeFilename,
		THRotatorSetting& outSetting,
		THRotatorFormatVersion& importedFormatVersion,
		std::list<std::wstring>& outMessages);

	static std::string GenerateIniAppName(const boost::filesystem::path& exeFilename);

	static boost::filesystem::path CreateIniFilePath(const boost::filesystem::path& processWorkingDir);

	static boost::filesystem::path CreateJsonFilePath(const boost::filesystem::path& processWorkingDir,
		const boost::filesystem::path& exeFilename);
};
