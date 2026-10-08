#ifndef DRIVER_BENCHMARK_H_DEFINED
#define DRIVER_BENCHMARK_H_DEFINED

#include <functional>
#include <string>
#include <vector>

#include "../core/Error.h"
#include "../Runtime/Sink.h"
#include "Options.h"
#include "StageTimes.h"

namespace Driver
{
    // Runs a whole file with the options and the output sink it is given, times each stage in the third argument
    using RunFn = std::function<Error::ErrorCode(Options const&, Runtime::OutputSink&, StageTimes&)>;

    // Runs each file with each back end (all three unless --mode was given): one run that checks the output
    // against the .out next to the file, then _options.benchRuns timed runs with the output dropped.
    // Prints a table per file. Returns the number of runs that failed
    int RunBenchmark(Options const& _options, std::vector<fs::path> const& _files, RunFn const& _run);

    // The memory line a run prints on stderr with --mem, built from the process peaks and the allocations of the run stage
    std::string FormatMemoryLine(Memory::ProcessMemory const& _process, Memory::Delta const& _run);

    // Runs each file with each back end (all three unless --mode was given) in a process of its own with --mem,
    // because the peak of the working set only grows. An empty program gives the base of each back end.
    // Prints a table per file. Returns the number of runs that failed
    int RunMemoryBenchmark(Options const& _options, std::vector<fs::path> const& _files);
}

#endif
