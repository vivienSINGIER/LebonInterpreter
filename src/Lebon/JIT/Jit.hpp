#ifndef JIT_HPP_DEFINED
#define JIT_HPP_DEFINED

#include "ExecutableMemory.h"
#include "Bytecode/BytecodeTests.h"
#include "Bytecode/Heap.hpp"
#include "Runtime.hpp"

namespace Jit
{
    struct JitCode
    {
        Bytecode::Heap heap;
        std::vector<uint64_t> globals;
        Jit::ExecutableMemory code;
        size_t entry = 0;
    
        JitCode() = default;
        JitCode(JitCode const&) = delete;
        JitCode& operator=(JitCode const&) = delete;

        // Runs the top level code, everything the program prints goes to _out.
        // The code must have been loaded : code.Entry() isn't null
        void Run(::Runtime::OutputSink& _out)
        {
            Runtime::g_heap = &heap;

            ::Runtime::OutputSink* previous = Runtime::g_out;
            Runtime::g_out = &_out;

            void* start = static_cast<uint8_t*>(code.Entry()) + entry;
            reinterpret_cast<void (*)()>(start)();

            Runtime::g_out = previous;
        }
    };
}

#endif

