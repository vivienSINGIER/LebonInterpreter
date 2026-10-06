#ifndef RUNTIME_ARENA_H_DEFINED
#define RUNTIME_ARENA_H_DEFINED

#include <cstddef>
#include <memory>
#include <vector>

namespace Runtime
{
    // Bump allocator. Memory is only given back all at once, by Reset or the destructor,
    // so the values allocated here never move and never need to be freed one by one.
    class Arena
    {
    public:
        static constexpr size_t DefaultChunkSize = 64 * 1024;

        explicit Arena(size_t _chunkSize = DefaultChunkSize);
        ~Arena() = default;

        Arena(Arena const&) = delete;
        Arena& operator=(Arena const&) = delete;

        // Never returns nullptr, _align must be a power of two
        void* Allocate(size_t _size, size_t _align = alignof(std::max_align_t));

        // Frees every allocation at once, the pointers given before become dangling
        void Reset();

        size_t BytesUsed() const { return m_bytesUsed; }

    private:
        struct Chunk
        {
            std::unique_ptr<std::byte[]> data;
            size_t size = 0;
            size_t used = 0;
        };

        // The last chunk is the one being filled
        std::vector<Chunk> m_chunks;
        size_t m_chunkSize;
        size_t m_bytesUsed = 0;

        static Chunk MakeChunk(size_t _size);
    };
}

#endif
