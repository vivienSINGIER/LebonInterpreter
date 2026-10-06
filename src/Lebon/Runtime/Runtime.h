#ifndef RUNTIME_RUNTIME_H_DEFINED
#define RUNTIME_RUNTIME_H_DEFINED

#include <cstddef>
#include <string>

#include "Arena.h"
#include "Sink.h"
#include "String.h"

// Runtime shared by the tree-walker, the VM and the JIT.
//
// Anything whose result can be observed (string building, printing) lives here
// so every back end produces byte-identical output. The entry points take plain
// pointers and scalars so JIT code can call them directly.
namespace Runtime
{
    // Must stay the type of NumberLiteral::value
    using Number = float;

    // What a running program needs from the outside
    struct Context
    {
        Arena* arena = nullptr;
        OutputSink* out = nullptr;
    };

    // Number format, the same for every back end:
    //   - the shortest digits that read back to the same Number, so 9 prints "9",
    //     12.5 prints "12.5" and 0.1 prints "0.1", never "9.000000"
    //   - fixed notation when 1e-6 <= |n| < 1e15, otherwise scientific ("1e+20", "1.5e-07")
    //   - -0 prints "0", infinities "inf" and "-inf", NaN "nan"
    constexpr size_t NumberBufferSize = 64;

    // Writes the text of _value to _buffer (no terminator), returns its length.
    // _buffer must hold NumberBufferSize chars, the text is always far shorter.
    size_t FormatNumber(Number _value, char* _buffer);
    std::string NumberToString(Number _value);

    // Booleans print as the canonical keywords of the language
    std::string_view BoolText(bool _value);

    // String functions working on the context arena
    String const* MakeString(Context* _ctx, std::string_view _text);
    String const* Concat(Context* _ctx, String const* _left, String const* _right);

    // One print function per type, each writes the value then a newline (afise)
    void PrintNumber(Context* _ctx, Number _value);
    void PrintString(Context* _ctx, String const* _value);
    void PrintBool(Context* _ctx, bool _value);
}

#endif
