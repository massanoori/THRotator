// (c) 2017 massanoori

#include "stdafx.h"
#include "THRotatorLog.h"
#include "THRotatorSystem.h"
#include "EncodingUtils.h"

#include <sstream>

namespace
{

boost::filesystem::path CreateTHRotatorLogFilePath()
{
	return GetTouhouPlayerDataDirectory() / L"throtator-log.txt";
}

std::ostream& GetLogOutputStream()
{
	static std::ostringstream temporaryOutputStream;

	try
	{
		static std::ofstream ofs;

		if (!ofs.is_open())
		{
			if (!(ofs.exceptions() & std::ios::failbit))
			{
				ofs.exceptions(std::ios::failbit);
			}

			ofs.open(CreateTHRotatorLogFilePath().generic_wstring());
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
		return temporaryOutputStream;
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
}
