#include "Benchmark.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "../core/FileHelper.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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

    std::string FormatMemoryLine(Memory::ProcessMemory const& _process, Memory::Delta const& _run)
    {
        return "[mem] peak_ws=" + std::to_string(_process.peakWorkingSet)
             + " peak_commit=" + std::to_string(_process.peakCommit)
             + " allocs=" + std::to_string(_run.allocs)
             + " alloc_bytes=" + std::to_string(_run.allocBytes)
             + " live_end=" + std::to_string(_run.liveChange)
             + " peak_live=" + std::to_string(_run.peakLive);
    }

    namespace
    {
        // What a child process printed on its --mem line, in bytes
        struct ChildMemory
        {
            std::string error;      // empty if the child ran the file and printed its line
            uint64_t peakWs = 0;
            uint64_t peakCommit = 0;
            uint64_t allocs = 0;
            uint64_t allocBytes = 0;
            int64_t liveEnd = 0;
            uint64_t peakLive = 0;
        };

        // Reads "key=value" words of the "[mem]" line
        bool ParseMemoryLine(std::string const& _output, ChildMemory& _out)
        {
            size_t const at = _output.find("[mem] ");
            if (at == std::string::npos)
                return false;

            size_t const end = _output.find('\n', at);
            std::string const line = _output.substr(at + 6, end == std::string::npos ? std::string::npos : end - at - 6);

            size_t found = 0;
            size_t pos = 0;
            while (pos < line.size())
            {
                size_t space = line.find(' ', pos);
                if (space == std::string::npos)
                    space = line.size();

                std::string const word = line.substr(pos, space - pos);
                size_t const equal = word.find('=');
                if (equal != std::string::npos)
                {
                    std::string const key = word.substr(0, equal);
                    long long const value = std::atoll(word.c_str() + equal + 1);

                    if (key == "peak_ws")           { _out.peakWs = static_cast<uint64_t>(value); found++; }
                    else if (key == "peak_commit")  { _out.peakCommit = static_cast<uint64_t>(value); found++; }
                    else if (key == "allocs")       { _out.allocs = static_cast<uint64_t>(value); found++; }
                    else if (key == "alloc_bytes")  { _out.allocBytes = static_cast<uint64_t>(value); found++; }
                    else if (key == "live_end")     { _out.liveEnd = value; found++; }
                    else if (key == "peak_live")    { _out.peakLive = static_cast<uint64_t>(value); found++; }
                }
                pos = space + 1;
            }
            return found == 6;
        }

#ifdef _WIN32
        // Runs the command line, returns what it wrote on stdout and stderr. False if the process can't start
        bool RunChild(std::wstring _commandLine, std::string& _output, unsigned long& _exitCode)
        {
            SECURITY_ATTRIBUTES security{};
            security.nLength = sizeof(security);
            security.bInheritHandle = TRUE;

            HANDLE read = nullptr;
            HANDLE write = nullptr;
            if (CreatePipe(&read, &write, &security, 0) == FALSE)
                return false;
            SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            startup.hStdOutput = write;
            startup.hStdError = write;

            PROCESS_INFORMATION process{};
            BOOL const started = CreateProcessW(nullptr, _commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
            CloseHandle(write);

            if (started == FALSE)
            {
                CloseHandle(read);
                return false;
            }

            char buffer[4096];
            DWORD count = 0;
            while (ReadFile(read, buffer, sizeof(buffer), &count, nullptr) && count > 0)
                _output.append(buffer, count);

            WaitForSingleObject(process.hProcess, INFINITE);
            DWORD code = 0;
            GetExitCodeProcess(process.hProcess, &code);
            _exitCode = code;

            CloseHandle(process.hProcess);
            CloseHandle(process.hThread);
            CloseHandle(read);
            return true;
        }

        ChildMemory MeasureChild(std::wstring const& _exe, fs::path const& _file, Mode _mode)
        {
            ChildMemory result;

            std::wstring command = L"\"" + _exe + L"\" \"" + _file.wstring() + L"\" --mode=";
            for (char c : std::string(ModeName(_mode)))
                command += static_cast<wchar_t>(c);
            command += L" --no-output --mem";

            std::string output;
            unsigned long exitCode = 0;
            if (RunChild(command, output, exitCode) == false)
                result.error = "can't start the process";
            else if (exitCode != 0)
                result.error = "exit code " + std::to_string(exitCode);
            else if (ParseMemoryLine(output, result) == false)
                result.error = "no memory line printed";

            return result;
        }

        std::wstring ExePath()
        {
            wchar_t path[MAX_PATH];
            DWORD const length = GetModuleFileNameW(nullptr, path, MAX_PATH);
            return std::wstring(path, length);
        }
#endif

        double Mib(uint64_t _bytes) { return static_cast<double>(_bytes) / (1024.0 * 1024.0); }
        double Kib(int64_t _bytes) { return static_cast<double>(_bytes) / 1024.0; }
    }

    int RunMemoryBenchmark(Options const& _options, std::vector<fs::path> const& _files)
    {
#ifdef _WIN32
        std::vector<Mode> modes;
        if (_options.modeSet)
            modes.push_back(_options.mode);
        else
            modes = { Mode::Tree, Mode::Vm, Mode::Jit };

#ifndef NDEBUG
        std::fputs("warning: debug build, the memory is not representative\n\n", stdout);
#endif

        // An empty program gives what a back end costs before running anything: the executable, the runtime, the code generators
        std::wstring const exe = ExePath();
        fs::path const empty = fs::temp_directory_path() / "lebon_membench_empty.lbn";
        {
            std::ofstream file(empty);
            file << "koz empty program finkoz\n";
        }

        std::vector<ChildMemory> bases;
        for (Mode mode : modes)
            bases.push_back(MeasureChild(exe, empty, mode));

        std::printf("peak ws = peak working set of the process, net = peak ws minus the empty program (MiB)\n");
        std::printf("allocs, alloc and live are those of the run stage only (KiB), from the global operator new\n");
        std::printf("the executable memory of the JIT is not counted by them, it is in peak ws\n");

        int failures = 0;
        for (fs::path const& file : _files)
        {
            std::printf("\n%s\n", file.filename().string().c_str());
            std::printf("  %-6s %9s %9s %10s %11s %10s %10s\n", "mode", "peak ws", "net", "allocs", "alloc KiB", "live KiB", "peak KiB");

            for (size_t i = 0; i < modes.size(); i++)
            {
                ChildMemory const m = MeasureChild(exe, file, modes[i]);
                if (m.error.empty() == false)
                {
                    failures++;
                    std::printf("  %-6s FAIL: %s\n", ModeName(modes[i]), m.error.c_str());
                    continue;
                }

                // The empty program can fail on a back end that can't run at all, there is no base then
                double net = bases[i].error.empty() ? Mib(m.peakWs) - Mib(bases[i].peakWs) : Mib(m.peakWs);
                std::printf("  %-6s %9.2f %9.2f %10llu %11.1f %10.1f %10.1f\n", ModeName(modes[i]),
                    Mib(m.peakWs), net, static_cast<unsigned long long>(m.allocs),
                    Kib(static_cast<int64_t>(m.allocBytes)), Kib(m.liveEnd), Kib(static_cast<int64_t>(m.peakLive)));
            }
        }

        std::error_code ignored;
        fs::remove(empty, ignored);

        if (failures != 0)
            std::printf("\n%d run(s) failed\n", failures);
        return failures;
#else
        (void)_options;
        (void)_files;
        std::fputs("--bench-mem is only available on Windows\n", stderr);
        return 1;
#endif
    }
}
