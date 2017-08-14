// (c) 2017 massanoori

#pragma once

enum class LogSeverity
{
	Fatal,
	Error,
	Warning,
	Info,
};

void SetTHRotatorLogPath(const std::wstring& logPath);

void OutputLogMessage(LogSeverity severity, const std::wstring& message);

template <typename... ArgsType>
void OutputLogMessagef(LogSeverity severity, const std::wstring& format, ArgsType&&... args)
{
	OutputLogMessage(severity, fmt::format(format, std::forward<ArgsType>(args)...));
}


