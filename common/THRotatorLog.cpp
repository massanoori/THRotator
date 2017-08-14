// (c) 2017 massanoori

#include "stdafx.h"
#include "THRotatorLog.h"
#include "EncodingUtils.h"

namespace
{

std::wstring LOG_FILE_PATH;
bool bFirstLog = true;

}

void SetTHRotatorLogPath(const std::wstring& logPath)
{
	if (logPath != LOG_FILE_PATH)
	{
		LOG_FILE_PATH = logPath;
		bFirstLog = true;
	}
}

void OutputLogMessage(LogSeverity severity, const std::wstring& message)
{
	std::ofstream ofs;
	if (bFirstLog)
	{
		ofs.open(LOG_FILE_PATH);
		bFirstLog = false;
	}
	else
	{
		ofs.open(LOG_FILE_PATH, std::ios::app);
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
