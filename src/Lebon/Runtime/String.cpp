#include "String.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Runtime
{
    namespace
    {
        // Allocates a string able to hold _length bytes, terminator already written
        String* Allocate(Arena& _arena, uint64_t _length)
        {
            if (_length > MaxStringLength)
            {
                // Can't be reported as a regular error from JIT code, and no
                // reasonable program gets there
                std::fprintf(stderr, "lebon runtime: string longer than 4 GiB\n");
                std::abort();
            }

            void* memory = _arena.Allocate(sizeof(String) + static_cast<size_t>(_length) + 1, alignof(String));
            String* str = static_cast<String*>(memory);
            str->length = static_cast<uint32_t>(_length);

            char* chars = reinterpret_cast<char*>(str + 1);
            chars[_length] = '\0';
            return str;
        }
    }

    String const* MakeString(Arena& _arena, std::string_view _text)
    {
        String* str = Allocate(_arena, _text.size());
        if (_text.empty() == false)
            std::memcpy(reinterpret_cast<char*>(str + 1), _text.data(), _text.size());
        return str;
    }

    String const* Concat(Arena& _arena, String const* _left, String const* _right)
    {
        uint64_t const length = uint64_t(_left->length) + _right->length;
        String* str = Allocate(_arena, length);

        char* chars = reinterpret_cast<char*>(str + 1);
        std::memcpy(chars, _left->Chars(), _left->length);
        std::memcpy(chars + _left->length, _right->Chars(), _right->length);
        return str;
    }

    bool Equals(String const* _left, String const* _right)
    {
        return _left->View() == _right->View();
    }
}
