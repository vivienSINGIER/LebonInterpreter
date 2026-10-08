#ifndef JIT_RUNTIME_HPP_DEFINED
#define JIT_RUNTIME_HPP_DEFINED

#include <iostream>

#include "Bytecode/Object.hpp"
#include "Bytecode/Heap.hpp"

namespace Jit
{
    namespace Runtime
    {
        static Bytecode::Heap* g_heap = nullptr;
        
        inline void PrintNumber(float _value) noexcept
        {
            std::cout << _value << "\n";
        }

        inline void PrintString(Bytecode::StringObj const* _text) noexcept
        {
            std::cout << _text->chars << "\n";
        }

        inline void PrintBool(bool _value) noexcept
        {
            std::cout << (_value ? "true" : "false") << "\n";
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
