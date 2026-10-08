#include "MemoryStats.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <malloc.h>
#include <new>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace
{
    // The program is single threaded, relaxed atomics only keep the counters well formed
    std::atomic<uint64_t> g_allocs{ 0 };
    std::atomic<uint64_t> g_allocBytes{ 0 };
    std::atomic<int64_t> g_live{ 0 };
    std::atomic<int64_t> g_peak{ 0 };

    void* Allocate(size_t _size)
    {
        void* block = std::malloc(_size != 0 ? _size : 1);
        if (block == nullptr)
            return nullptr;

        // The size the allocator really reserved, the same one is read back when the block is freed
        size_t const size = _msize(block);
        g_allocs.fetch_add(1, std::memory_order_relaxed);
        g_allocBytes.fetch_add(size, std::memory_order_relaxed);

        int64_t const live = g_live.fetch_add(static_cast<int64_t>(size), std::memory_order_relaxed) + static_cast<int64_t>(size);
        if (live > g_peak.load(std::memory_order_relaxed))
            g_peak.store(live, std::memory_order_relaxed);

        return block;
    }

    void* AllocateOrThrow(size_t _size)
    {
        void* block = Allocate(_size);
        if (block == nullptr)
            throw std::bad_alloc();
        return block;
    }

    void Free(void* _block) noexcept
    {
        if (_block == nullptr)
            return;

        g_live.fetch_sub(static_cast<int64_t>(_msize(_block)), std::memory_order_relaxed);
        std::free(_block);
    }
}

void* operator new(size_t _size)                                      { return AllocateOrThrow(_size); }
void* operator new[](size_t _size)                                    { return AllocateOrThrow(_size); }
void* operator new(size_t _size, std::nothrow_t const&) noexcept      { return Allocate(_size); }
void* operator new[](size_t _size, std::nothrow_t const&) noexcept    { return Allocate(_size); }

void operator delete(void* _block) noexcept                           { Free(_block); }
void operator delete[](void* _block) noexcept                         { Free(_block); }
void operator delete(void* _block, size_t) noexcept                   { Free(_block); }
void operator delete[](void* _block, size_t) noexcept                 { Free(_block); }
void operator delete(void* _block, std::nothrow_t const&) noexcept    { Free(_block); }
void operator delete[](void* _block, std::nothrow_t const&) noexcept  { Free(_block); }

namespace Driver::Memory
{
    void Delta::Add(Delta const& _other)
    {
        // The peak of two successive stages: the second one starts where the first one ended
        peakLive = std::max(peakLive, static_cast<uint64_t>(std::max<int64_t>(0, liveChange + static_cast<int64_t>(_other.peakLive))));
        allocs += _other.allocs;
        allocBytes += _other.allocBytes;
        liveChange += _other.liveChange;
    }

    Snapshot Take()
    {
        Snapshot snapshot;
        snapshot.allocs = g_allocs.load(std::memory_order_relaxed);
        snapshot.allocBytes = g_allocBytes.load(std::memory_order_relaxed);
        snapshot.live = g_live.load(std::memory_order_relaxed);

        g_peak.store(snapshot.live, std::memory_order_relaxed);
        return snapshot;
    }

    Delta Since(Snapshot const& _from)
    {
        Delta delta;
        delta.allocs = g_allocs.load(std::memory_order_relaxed) - _from.allocs;
        delta.allocBytes = g_allocBytes.load(std::memory_order_relaxed) - _from.allocBytes;
        delta.liveChange = g_live.load(std::memory_order_relaxed) - _from.live;
        delta.peakLive = static_cast<uint64_t>(std::max<int64_t>(0, g_peak.load(std::memory_order_relaxed) - _from.live));
        return delta;
    }

    ProcessMemory Process()
    {
        ProcessMemory memory;
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS counters{};
        counters.cb = sizeof(counters);
        if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
        {
            memory.workingSet = counters.WorkingSetSize;
            memory.peakWorkingSet = counters.PeakWorkingSetSize;
            memory.peakCommit = counters.PeakPagefileUsage;
        }
#endif
        return memory;
    }
}
