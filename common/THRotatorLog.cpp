// (c) 2017 massanoori

#include "stdafx.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"
#include "EncodingUtils.h"

namespace
{

bool bFirstLog = true;

boost::filesystem::path CreateTHRotatorLogFilePath()
{
	return GetTouhouPlayerDataDirectory() / L"throtLog.txt";
}

}

void OutputLogMessage(LogSeverity severity, const std::wstring& message)
{
	std::ofstream ofs;
	if (bFirstLog)
	{
		ofs.open(CreateTHRotatorLogFilePath().generic_wstring());
		bFirstLog = false;
	}
	else
	{
		ofs.open(CreateTHRotatorLogFilePath().generic_wstring(), std::ios::app);
	}

	auto now = std::time(nullptr);
	tm localNow;
	if (localtime_s(&localNow, &now) == 0)
	{
		std::vector<char> timeStringBuffer(1);
		while (std::strftime(timeStringBuffer.data(), timeStringBuffer.size(), "%F %T ", &localNow) == 0)
		{
			timeStringBuffer.resize(timeStringBuffer.size() * 2);
		}

		ofs << timeStringBuffer.data();
	}

	switch (severity)
	{
	case LogSeverity::Fatal:
		ofs << "Fatal:   ";
		break;

	case LogSeverity::Error:
		ofs << "Error:   ";
		break;

	case LogSeverity::Warning:
		ofs << "Warning: ";
		break;

	case LogSeverity::Info:
		ofs << "Info:    ";
		break;
	}

	ofs << ConvertFromUnicodeToUtf8(message) << std::endl;
}
