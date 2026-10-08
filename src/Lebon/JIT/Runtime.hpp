#ifndef JIT_RUNTIME_HPP_DEFINED
#define JIT_RUNTIME_HPP_DEFINED

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "Bytecode/Object.hpp"
#include "Bytecode/Heap.hpp"
#include "runtime/Runtime.h"

namespace Jit
{
    // The Runtime of the whole interpreter is written ::Runtime in here, Runtime alone is this namespace
    namespace Runtime
    {
        // inline : one variable for the whole program, static would give each .cpp its own
        inline Bytecode::Heap* g_heap = nullptr;

        // Where afise writes. The console until a run gives its own sink, see JitCode::Run
        inline ::Runtime::ConsoleSink g_console;
        inline ::Runtime::OutputSink* g_out = &g_console;

        // Same text as Bytecode::ToString, what the VM and the tree-walking interpreter print :
        // %g is how a stream writes a float, and the booleans are true and false
        inline void PrintNumber(float _value) noexcept
        {
            char buffer[64];
            int length = std::snprintf(buffer, sizeof(buffer), "%g\n", _value);
            if (length > 0)
                g_out->Write({ buffer, static_cast<size_t>(length) });
        }

        inline void PrintString(Bytecode::StringObj const* _text) noexcept
        {
            g_out->Write(_text->chars);
            g_out->Write("\n");
        }

        inline void PrintBool(bool _value) noexcept
        {
            g_out->Write(_value ? "true\n" : "false\n");
        }

        inline Bytecode::StringObj* Concat(Bytecode::StringObj* _str1, Bytecode::StringObj* _str2) noexcept
        {
            return g_heap->Intern(_str1->chars + _str2->chars);
        }

        inline uint32_t CompareString(Bytecode::StringObj* _str1, Bytecode::StringObj* _str2) noexcept
        {
            if (_str1->chars.length() != _str2->chars.length())
                return 0;

            for (uint32_t i = 0; i < _str1->chars.length(); i++)
            {
                if (_str1->chars[i] != _str2->chars[i])
                    return 0;
            }

            return 1;
        }
    }
}

#endif
