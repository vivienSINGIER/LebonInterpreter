#ifndef JIT_EXECUTABLE_MEMORY_H_DEFINED
#define JIT_EXECUTABLE_MEMORY_H_DEFINED

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Jit
{
    class ExecutableMemory
    {
    public:
        ExecutableMemory() = default;
        ~ExecutableMemory();

        ExecutableMemory(ExecutableMemory const&) = delete;
        ExecutableMemory& operator=(ExecutableMemory const&) = delete;
        
        bool Load(std::vector<uint8_t> const& _code);

        void* Entry() const { return m_memory; }
        size_t Size() const { return m_size; }
        
        template <typename Fn>
        Fn As() const { return reinterpret_cast<Fn>(m_memory); }

    private:
        void* m_memory = nullptr;
        size_t m_size = 0;

        void Release();
    };
}

#endif
