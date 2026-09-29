#ifndef LOGGER_H_DEFINED
#define LOGGER_H_DEFINED

#include <string>

enum class LogType : int
{
    Help, Info, PromptInfo, Prompt, Warning, Error
};

namespace Log
{
    void Log(LogType _logType, std::string const& _message);
    
    std::string GetLogColor(LogType _type);
}

#endif
