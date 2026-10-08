#include "Benchmark.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "../core/FileHelper.h"


namespace Driver
{
    namespace
    {
        struct Measurement
        {
            std::string error;      // empty if the back end ran the file and gave the expected output
            double front = 0;       // median of everything but the run stage: lex, parse, analyse, compile
            double runMin = 0;
            double runMedian = 0;
        };

        double Median(std::vector<double>& _values)
        {
            std::sort(_values.begin(), _values.end());
            return _values[_values.size() / 2];
        }

        // Same text whatever the line endings and the trailing newline
        std::string Normalize(std::string _text)
        {
            _text.erase(std::remove(_text.begin(), _text.end(), '\r'), _text.end());
            while (_text.empty() == false && (_text.back() == '\n' || _text.back() == ' '))
                _text.pop_back();
            return _text;
        }

        Measurement Measure(Options _options, fs::path const& _file, RunFn const& _run)
        {
            Measurement result;
            _options.file = _file;
            _options.noOutput = false;

            // First run: the output is kept and checked, it also warms the back end up
            Runtime::BufferSink captured;
            StageTimes first;
            Error::ErrorCode const code = _run(_options, captured, first);
            if (code != Error::ErrorCode::Ok)
            {
                result.error = "exit code " + std::to_string(static_cast<int>(code));
                return result;
            }

            fs::path expectedFile = _file;
            expectedFile.replace_extension(".out");
            if (fs::exists(expectedFile))
            {
                std::string expected;
                if (Error e = FileHelper::ReadFile(expectedFile, expected))
                {
                    result.error = e.Format();
                    return result;
                }
                if (Normalize(captured.Str()) != Normalize(expected))
                {
                    result.error = "wrong output";
                    return result;
                }
            }

            Runtime::NullSink null;
            std::vector<double> runs, fronts;
            for (int i = 0; i < _options.benchRuns; i++)
            {
                StageTimes times;
                if (_run(_options, null, times) != Error::ErrorCode::Ok)
                {
                    result.error = "failed on run " + std::to_string(i + 1);
                    return result;
                }
                runs.push_back(times.Of("run"));
                fronts.push_back(times.Total() - times.Of("run"));
            }

            result.runMin = *std::min_element(runs.begin(), runs.end());
            result.runMedian = Median(runs);
            result.front = Median(fronts);
            return result;
        }
    }

    int RunBenchmark(Options const& _options, std::vector<fs::path> const& _files, RunFn const& _run)
    {
        std::vector<Mode> modes;
        if (_options.modeSet)
            modes.push_back(_options.mode);
        else
            modes = { Mode::Tree, Mode::Vm, Mode::Jit };

#ifndef NDEBUG
        std::fputs("warning: debug build, the times are not representative\n\n", stdout);
#endif
        std::printf("%d timed run(s) per back end, times in ms (front = lex + parse + analyse + compile)\n", _options.benchRuns);

        int failures = 0;
        for (fs::path const& file : _files)
        {
            std::printf("\n%s\n", file.filename().string().c_str());
            std::printf("  %-6s %10s %10s %10s\n", "mode", "front", "run min", "run med");

            for (Mode mode : modes)
            {
                Options options = _options;
                options.mode = mode;

                Measurement const m = Measure(options, file, _run);
                if (m.error.empty() == false)
                {
                    failures++;
                    std::printf("  %-6s FAIL: %s\n", ModeName(mode), m.error.c_str());
                    continue;
                }

                std::printf("  %-6s %10.3f %10.3f %10.3f\n", ModeName(mode), m.front, m.runMin, m.runMedian);
            }
        }

        std::printf("\n%s\n", failures == 0 ? "all runs gave the expected output" : (std::to_string(failures) + " run(s) failed").c_str());
        return failures;
    }
}
