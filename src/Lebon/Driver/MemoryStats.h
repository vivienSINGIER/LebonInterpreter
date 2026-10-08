#ifndef DRIVER_MEMORYSTATS_H_DEFINED
#define DRIVER_MEMORYSTATS_H_DEFINED

#include <cstddef>
#include <cstdint>

// Every allocation of the program goes through the global operator new, which counts it (MemoryStats.cpp).
// The counters are cheap, they always run, a measure is the difference between two snapshots.
namespace Driver::Memory
{
    struct Snapshot
    {
        uint64_t allocs = 0;        // calls to operator new since the start
        uint64_t allocBytes = 0;    // bytes requested since the start
        int64_t live = 0;           // bytes allocated and not freed yet
    };

    // What happened between a snapshot and now
    struct Delta
    {
        uint64_t allocs = 0;
        uint64_t allocBytes = 0;
        int64_t liveChange = 0;     // bytes still allocated at the end minus at the start
        uint64_t peakLive = 0;      // most bytes allocated at once above the starting point

        void Add(Delta const& _other);
    };

    // Also restarts the peak, so peakLive of the next Since() only covers what comes after
    Snapshot Take();
    Delta Since(Snapshot const& _from);

    // Memory the system gave to the process, in bytes. The peaks are the highest since the process started
    struct ProcessMemory
    {
        size_t workingSet = 0;
        size_t peakWorkingSet = 0;
        size_t peakCommit = 0;
    };

    ProcessMemory Process();
}

#endif
