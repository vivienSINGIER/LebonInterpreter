#include "Logger.h"

#include <iostream>

void Log::Log(LogType _logType, std::string const& _message)
{
    if (_logType == LogType::Error)
        std::cerr << GetLogColor(LogType::Error) << _message << "\033[0m";
    else
        std::cout << GetLogColor(_logType) << _message << "\033[0m";
}

std::string Log::GetLogColor(LogType _type)
{
    switch (_type)
    {
    case LogType::Help:         return "\033[90m"; // grey
    case LogType::Prompt:       return "\033[94m"; // bright blue
    case LogType::PromptInfo:   return "\033[34m"; // bright blue
    case LogType::Info:         return "\033[32m"; // green
    case LogType::Warning:      return "\033[33m"; // yellow
    case LogType::Error:        return "\033[31m"; // red
    }
    return "\033[0m";
}
