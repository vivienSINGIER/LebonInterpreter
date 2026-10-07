#ifndef JIT_HPP_DEFINED
#define JIT_HPP_DEFINED

#include "ExecutableMemory.h"
#include "Bytecode/BytecodeTests.h"
#include "Bytecode/Heap.hpp"

struct JitCode
{
    Bytecode::Heap heap;
    std::vector<uint64_t> globals;
    Jit::ExecutableMemory code;
    size_t entry = 0;
    
    JitCode() = default;
    JitCode(JitCode const&) = delete;
    JitCode& operator=(JitCode const&) = delete;
};

#endif

