#include "Runtime.h"

#include <charconv>
#include <cmath>
#include <cstring>

namespace Runtime
{
    namespace
    {
        size_t Copy(char const* _text, char* _buffer)
        {
            size_t const length = std::strlen(_text);
            std::memcpy(_buffer, _text, length);
            return length;
        }
    }

    size_t FormatNumber(Number _value, char* _buffer)
    {
        if (std::isnan(_value))
            return Copy("nan", _buffer);
        if (std::isinf(_value))
            return Copy(_value < 0 ? "-inf" : "inf", _buffer);
        if (_value == 0)
            return Copy("0", _buffer); // also -0

        Number const magnitude = std::fabs(_value);
        std::chars_format const format = magnitude >= Number(1e-6) && magnitude < Number(1e15)
            ? std::chars_format::fixed
            : std::chars_format::scientific;

        // Without a precision, to_chars gives the shortest round-trip digits
        std::to_chars_result const result = std::to_chars(_buffer, _buffer + NumberBufferSize, _value, format);
        return static_cast<size_t>(result.ptr - _buffer);
    }

    std::string NumberToString(Number _value)
    {
        char buffer[NumberBufferSize];
        return std::string(buffer, FormatNumber(_value, buffer));
    }

    std::string_view BoolText(bool _value)
    {
        // "vré" and "fo", spelled as UTF-8 bytes so it doesn't depend on the source encoding
        return _value ? "vr\xC3\xA9" : "fo";
    }

    void PrintNumber(Context* _ctx, Number _value)
    {
        char buffer[NumberBufferSize + 1];
        size_t length = FormatNumber(_value, buffer);
        buffer[length++] = '\n';
        _ctx->out->Write({ buffer, length });
    }

    void PrintString(Context* _ctx, std::string const& _value)
    {
        _ctx->out->Write(_value);
        _ctx->out->Write("\n");
    }

    void PrintBool(Context* _ctx, bool _value)
    {
        _ctx->out->Write(BoolText(_value));
        _ctx->out->Write("\n");
    }
}
