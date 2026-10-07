#ifndef JIT_EXECUTABLE_MEMORY_H_DEFINED
#define JIT_EXECUTABLE_MEMORY_H_DEFINED

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Jit
{
    // Owns the pages the machine code runs from.
    // The pages are never writable and executable at the same time : written first, then switched to execute-read
    class ExecutableMemory
    {
    public:
        ExecutableMemory() = default;
        ~ExecutableMemory();

        ExecutableMemory(ExecutableMemory const&) = delete;
        ExecutableMemory& operator=(ExecutableMemory const&) = delete;

        // Copies the code into fresh pages and makes them executable, false if the system refused
        bool Load(std::vector<uint8_t> const& _code);

        void* Entry() const { return m_memory; }
        size_t Size() const { return m_size; }

        // The code seen as a function, the caller is the one who knows its signature
        template <typename Fn>
        Fn As() const { return reinterpret_cast<Fn>(m_memory); }

    private:
        void* m_memory = nullptr;
        size_t m_size = 0;

        void Release();
    };
}

#endif
