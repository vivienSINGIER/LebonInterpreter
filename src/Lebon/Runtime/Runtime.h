#ifndef RUNTIME_RUNTIME_H_DEFINED
#define RUNTIME_RUNTIME_H_DEFINED

#include <cstddef>
#include <string>

#include "Sink.h"
#include "String.h"

namespace Runtime
{
    using Number = float;
    
    struct Context
    {
        OutputSink* out = nullptr;
    };
    
    constexpr size_t NumberBufferSize = 64;
    
    size_t FormatNumber(Number _value, char* _buffer);
    std::string NumberToString(Number _value);
    
    std::string_view BoolText(bool _value);
    
    void PrintNumber(OutputSink* _ctx, Number _value);
    void PrintString(OutputSink* _ctx, std::string const& _value);
    void PrintBool(OutputSink* _ctx, bool _value);
}

#endif
