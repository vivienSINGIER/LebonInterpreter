#include "core/Error.h"

namespace
{
    char const* Label(Error::ErrorCode _code)
    {
        switch (_code)
        {
        case Error::ErrorCode::Ok:         return "ok";
        case Error::ErrorCode::Lexical:    return "lexical error";
        case Error::ErrorCode::Syntax:     return "syntax error";
        case Error::ErrorCode::Semantics:  return "semantics error";
        case Error::ErrorCode::Execution:      return "execution error";
        }
        return "error";
    }
}

ErrorManager * ErrorManager::singleton = nullptr;

std::string Error::Format() const
{
    if (IsOk())
        return "ok";

    std::string out = Label(code);
    out += ": ";
    out += message;

    for (std::string const& detail : details)
    {
        out += "\n  - ";
        out += detail;
    }

    return out;
}

Error Error::Ok()
{
    return Error();
}

Error Error::Lexical(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Lexical;
    e.message = std::move(_message);
    return e;
}

Error Error::Syntax(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Syntax;
    e.message = std::move(_message);
    return e;
}

Error Error::Semantics(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Semantics;
    e.message = std::move(_message);
    return e;
}

Error Error::Execution(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Execution;
    e.message = std::move(_message);
    return e;
}

ErrorManager* ErrorManager::GetErrorManager()
{
    if ( singleton != nullptr )
        delete singleton;
        
    singleton = new ErrorManager();
    return singleton;
}

void ErrorManager::LogError(Error const& _error)
{
    if (!_error)
        return;
        
    Log::Log(LogType::Error, _error.message);
    numberOfErrors++;
}

uint16_t ErrorManager::GetNumberOfErrors()
{
    return numberOfErrors;
}
