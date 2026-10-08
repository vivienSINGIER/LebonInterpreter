#ifndef DRIVER_STAGETIMES_H_DEFINED
#define DRIVER_STAGETIMES_H_DEFINED

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "MemoryStats.h"

namespace Driver
{
    // Time and allocations of each stage of a run (lex, parse, analyse, compile, run)
    class StageTimes
    {
    public:
        template <typename F>
        void Measure(char const* _stage, F&& _work)
        {
            Memory::Snapshot const before = Memory::Take();
            auto const start = std::chrono::steady_clock::now();
            _work();
            auto const end = std::chrono::steady_clock::now();

            m_stages.push_back({ _stage, std::chrono::duration<double, std::milli>(end - start).count(), Memory::Since(before) });
        }

        // Time of one stage in ms, 0 if it didn't run
        double Of(char const* _stage) const
        {
            double total = 0;
            for (Stage const& stage : m_stages)
            {
                if (std::strcmp(stage.name, _stage) == 0)
                    total += stage.ms;
            }
            return total;
        }

        double Total() const
        {
            double total = 0;
            for (Stage const& stage : m_stages)
                total += stage.ms;
            return total;
        }

        // Allocations of one stage, all zero if it didn't run
        Memory::Delta MemoryOf(char const* _stage) const
        {
            Memory::Delta total;
            for (Stage const& stage : m_stages)
            {
                if (std::strcmp(stage.name, _stage) == 0)
                    total.Add(stage.memory);
            }
            return total;
        }

        // Printed on stderr with --time
        void Print() const
        {
            for (Stage const& stage : m_stages)
                std::fprintf(stderr, "[time] %-8s %10.3f ms\n", stage.name, stage.ms);
            std::fprintf(stderr, "[time] %-8s %10.3f ms\n", "total", Total());
        }

    private:
        struct Stage
        {
            char const* name;
            double ms;
            Memory::Delta memory;
        };

        std::vector<Stage> m_stages;
    };
}

#endif
