#include "ExecutableMemory.h"

#include <cstring>

#include <windows.h>

namespace Jit
{
    ExecutableMemory::~ExecutableMemory()
    {
        Release();
    }

    bool ExecutableMemory::Load(std::vector<uint8_t> const& _code)
    {
        Release();
        if (_code.empty())
            return false;

        void* memory = VirtualAlloc(nullptr, _code.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (memory == nullptr)
            return false;

        std::memcpy(memory, _code.data(), _code.size());

        DWORD oldProtect = 0;
        if (VirtualProtect(memory, _code.size(), PAGE_EXECUTE_READ, &oldProtect) == FALSE)
        {
            VirtualFree(memory, 0, MEM_RELEASE);
            return false;
        }
        
        FlushInstructionCache(GetCurrentProcess(), memory, _code.size());

        m_memory = memory;
        m_size = _code.size();
        return true;
    }

    void ExecutableMemory::Release()
    {
        if (m_memory != nullptr)
            VirtualFree(m_memory, 0, MEM_RELEASE);

        m_memory = nullptr;
        m_size = 0;
    }
}
