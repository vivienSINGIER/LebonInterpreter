#ifndef DRIVER_STAGETIMES_H_DEFINED
#define DRIVER_STAGETIMES_H_DEFINED

#include <chrono>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace Driver
{
    // Time spent in each stage of a run (lex, parse, analyse, compile, run)
    class StageTimes
    {
    public:
        template <typename F>
        void Measure(char const* _stage, F&& _work)
        {
            auto const start = std::chrono::steady_clock::now();
            _work();
            auto const end = std::chrono::steady_clock::now();
            m_stages.emplace_back(_stage, std::chrono::duration<double, std::milli>(end - start).count());
        }

        // Time of one stage in ms, 0 if it didn't run
        double Of(char const* _stage) const
        {
            double total = 0;
            for (auto const& [stage, ms] : m_stages)
            {
                if (std::strcmp(stage, _stage) == 0)
                    total += ms;
            }
            return total;
        }

        double Total() const
        {
            double total = 0;
            for (auto const& stage : m_stages)
                total += stage.second;
            return total;
        }

        // Printed on stderr with --time
        void Print() const
        {
            for (auto const& [stage, ms] : m_stages)
                std::fprintf(stderr, "[time] %-8s %10.3f ms\n", stage, ms);
            std::fprintf(stderr, "[time] %-8s %10.3f ms\n", "total", Total());
        }

    private:
        std::vector<std::pair<char const*, double>> m_stages;
    };
}

#endif
