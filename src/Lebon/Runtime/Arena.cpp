#include "Arena.h"

#include <cstdint>

namespace Runtime
{
    namespace
    {
        // Bytes to skip from _address so it becomes a multiple of _align
        size_t Padding(std::byte const* _address, size_t _align)
        {
            uintptr_t const address = reinterpret_cast<uintptr_t>(_address);
            return (_align - (address & (_align - 1))) & (_align - 1);
        }
    }

    Arena::Arena(size_t _chunkSize) : m_chunkSize(_chunkSize)
    {
    }

    Arena::Chunk Arena::MakeChunk(size_t _size)
    {
        Chunk chunk;
        chunk.data = std::make_unique_for_overwrite<std::byte[]>(_size);
        chunk.size = _size;
        return chunk;
    }

    void* Arena::Allocate(size_t _size, size_t _align)
    {
        if (m_chunks.empty() == false)
        {
            Chunk& current = m_chunks.back();
            size_t const padding = Padding(current.data.get() + current.used, _align);
            if (current.used + padding + _size <= current.size)
            {
                std::byte* result = current.data.get() + current.used + padding;
                current.used += padding + _size;
                m_bytesUsed += _size;
                return result;
            }
        }

        // Worst case padding, so the allocation always fits in the new chunk
        size_t const needed = _size + _align - 1;

        if (needed > m_chunkSize / 4)
        {
            // Big allocation: own chunk, slipped under the current one so the
            // space left in the current one is still used
            Chunk big = MakeChunk(needed);
            std::byte* result = big.data.get() + Padding(big.data.get(), _align);
            big.used = big.size;

            if (m_chunks.empty())
                m_chunks.push_back(std::move(big));
            else
                m_chunks.insert(m_chunks.end() - 1, std::move(big));

            m_bytesUsed += _size;
            return result;
        }

        m_chunks.push_back(MakeChunk(m_chunkSize));
        return Allocate(_size, _align);
    }

    void Arena::Reset()
    {
        m_chunks.clear();
        m_bytesUsed = 0;
    }
}
