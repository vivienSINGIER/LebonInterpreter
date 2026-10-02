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
        case Error::ErrorCode::Io:           return "io error";
        }
        return "error";
    }
}

std::string Error::Format() const
{
    if (IsOk())
        return "ok";

    std::string out;
    if (line != 0 && col != 0)
        out = "(" + std::to_string(line) + ", " + std::to_string(col) + "): ";
    
    out += Label(code);
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

Error Error::Lexical(std::string _message, uint32_t _line, uint32_t _col)
{
    Error e;
    e.code    = ErrorCode::Lexical;
    e.message = std::move(_message);
    e.line    = _line;
    e.col     = _col;
    return e;
}

Error Error::Syntax(std::string _message, uint32_t _line, uint32_t _col)
{
    Error e;
    e.code    = ErrorCode::Syntax;
    e.message = std::move(_message);
    e.line    = _line;
    e.col     = _col;
    return e;
}

Error Error::Semantics(std::string _message, uint32_t _line, uint32_t _col)
{
    Error e;
    e.code    = ErrorCode::Semantics;
    e.message = std::move(_message);
    e.line    = _line;
    e.col     = _col;
    return e;
}

Error Error::Execution(std::string _message, uint32_t _line, uint32_t _col)
{
    Error e;
    e.code    = ErrorCode::Execution;
    e.message = std::move(_message);
    e.line    = _line;
    e.col     = _col;
    return e;
}

Error Error::Io(std::string _message, fs::path _path)
{
    Error e;
    e.code    = ErrorCode::Io;
    e.message = std::move(_message);
    e.path    = std::move(_path);
    return e;
}

ErrorManager* ErrorManager::GetErrorManager()
{
    if (m_instance == nullptr)
        m_instance = new ErrorManager();

    return m_instance;
}

void ErrorManager::Clear()
{
    delete m_instance;
    m_instance = nullptr;
}

void ErrorManager::LogError(Error const& _error)
{
    if (!_error)
        return;

    Log::Log(LogType::Error, _error.Format());
    Log::Log(LogType::Prompt, "\n");
    GetErrorManager()->m_errors.push_back(_error);
}

int ErrorManager::Code()
{
    auto const& e = Errors();
    return e.empty() ? 0 : static_cast<int>(e.front().code);
}
