#ifndef RUNTIME_STRING_H_DEFINED
#define RUNTIME_STRING_H_DEFINED

#include <cstdint>
#include <string_view>

#include "Arena.h"

namespace Runtime
{
    // Immutable string, shared by every back end.
    //
    // Memory layout, one contiguous block allocated in an Arena:
    //   [ uint32_t length ][ length bytes of UTF-8 ][ '\0' ]
    // The terminator isn't counted in the length, it's only there so the
    // characters can be handed to C functions as they are.
    //
    // A string is never modified after creation: operations build a new one.
    // Back ends pass strings around as `String const*`.
    struct String
    {
        uint32_t length;

        char const* Chars() const { return reinterpret_cast<char const*>(this + 1); }
        std::string_view View() const { return { Chars(), length }; }
    };

    static_assert(sizeof(String) == sizeof(uint32_t), "the characters must directly follow the length");

    constexpr uint64_t MaxStringLength = UINT32_MAX;

    // Copies _text into a new string allocated in _arena
    String const* MakeString(Arena& _arena, std::string_view _text);

    // New string holding _left then _right, the operands are left untouched
    String const* Concat(Arena& _arena, String const* _left, String const* _right);

    bool Equals(String const* _left, String const* _right);
}

#endif
