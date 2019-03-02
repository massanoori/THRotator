// (c) 2017 massanoori

#include "stdafx.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"
#include "EncodingUtils.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace
{

std::filesystem::path CreateTHRotatorLogFilePath()
{
	return GetTouhouPlayerDataDirectory() / L"throtator-log.txt";
}

std::ostream& GetLogOutputStream()
{
	try
	{
		static std::ofstream ofs;
		static std::ostringstream temporaryOutputStream;

		if (!ofs.is_open())
		{
			auto logFilePath = CreateTHRotatorLogFilePath().generic_wstring();

			// Calling CreateTHRotatorLogFilePath() may open ofs in a recurrent call, so re-check ofs's status.
			if (!ofs.is_open())
			{
				ofs.open(logFilePath);
			}
		}

		if (!ofs.is_open())
		{
			return temporaryOutputStream;
		}

		if (temporaryOutputStream.tellp() > std::streampos(0))
		{
			ofs << temporaryOutputStream.str();
			temporaryOutputStream = std::ostringstream();
		}

		return ofs;
	}
	catch (const std::ios::failure&)
	{
		return std::cout;
	}
}

}

void OutputLogMessage(LogSeverity severity, const std::string& message)
{
	auto& logStream = GetLogOutputStream();

	auto now = std::time(nullptr);
	tm localNow;
	if (localtime_s(&localNow, &now) == 0)
	{
		std::vector<char> timeStringBuffer(1);
		while (std::strftime(timeStringBuffer.data(), timeStringBuffer.size(), "%F %T ", &localNow) == 0)
		{
			timeStringBuffer.resize(timeStringBuffer.size() * 2);
		}

		logStream << timeStringBuffer.data();
	}

	switch (severity)
	{
	case LogSeverity::Fatal:
		logStream << "Fatal:   ";
		break;

	case LogSeverity::Error:
		logStream << "Error:   ";
		break;

	case LogSeverity::Warning:
		logStream << "Warning: ";
		break;

	case LogSeverity::Info:
		logStream << "Info:    ";
		break;
	}

	logStream << message << std::endl;
	logStream.flush();
}
