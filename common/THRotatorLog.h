// (c) 2017 massanoori

#pragma once

enum class LogSeverity
{
	Fatal,
	Error,
	Warning,
	Info,
};

void OutputLogMessage(LogSeverity severity, const std::string& message);

template <typename... ArgsType>
void OutputLogMessagef(LogSeverity severity, const std::string& format, ArgsType&&... args)
{
	try
	{
		OutputLogMessage(severity, fmt::format(format, std::forward<ArgsType>(args)...));
	}
	catch (const fmt::FormatError&)
	{
		OutputLogMessage(LogSeverity::Error, "Failed to parse log string.");
		OutputLogMessage(LogSeverity::Error,
			std::string("Original message: '") + format + "'");
	}
}

