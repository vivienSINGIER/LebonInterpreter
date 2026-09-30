#ifndef CORE_ERROR_H
#define CORE_ERROR_H

#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "Logger.h"

struct Error
{
    enum class ErrorCode : int
    {
        Ok = 0,
        Lexical = 1,
        Syntax = 2,
        Semantics = 3,
        Execution = 4,
    };

    ErrorCode code = ErrorCode::Ok;
    std::string message;
    std::vector<std::string> details;
    uint32_t col, line;
    
    bool IsOk() const { return code == ErrorCode::Ok; }
    explicit operator bool() const { return !IsOk(); }
    int Exit() const { return static_cast<int>(code); }
    
    std::string Format() const;

    static Error Ok();
    static Error Lexical(std::string _message, uint32_t _line, uint32_t _col);
    static Error Syntax(std::string _message, uint32_t _line, uint32_t _col);
    static Error Semantics(std::string _message, uint32_t _line, uint32_t _col);
    static Error Execution(std::string _message, uint32_t _line, uint32_t _col);
};

class ErrorManager
{
    inline static ErrorManager* m_instance = nullptr;
    uint16_t numberOfErrors = 0;

public:
    static ErrorManager* GetErrorManager();
    
    static void LogError(Error const& _error);

    uint16_t GetNumberOfErrors();
};



#endif
