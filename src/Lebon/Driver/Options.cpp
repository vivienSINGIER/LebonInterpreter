#include "Options.h"

#include <charconv>

namespace Driver
{
    namespace
    {
        std::optional<Mode> ParseMode(std::string_view _name)
        {
            if (_name == "tree") return Mode::Tree;
            if (_name == "vm")   return Mode::Vm;
            if (_name == "jit")  return Mode::Jit;
            return std::nullopt;
        }
    }

    char const* ModeName(Mode _mode)
    {
        switch (_mode)
        {
        case Mode::Tree: return "tree";
        case Mode::Vm:   return "vm";
        case Mode::Jit:  return "jit";
        }
        return "?";
    }

    bool ParseCommandLine(int _argc, char** _argv, Options& _options, std::string& _error)
    {
        bool hasFile = false;

        for (int i = 1; i < _argc; i++)
        {
            std::string_view const arg = _argv[i];

            if (arg == "-h" || arg == "--help") { _options.help = true; continue; }
            if (arg == "--dump-tokens")         { _options.dumpTokens = true; continue; }
            if (arg == "--dump-ast")            { _options.dumpAst = true; continue; }
            if (arg == "--dump-bytecode")       { _options.dumpBytecode = true; continue; }
            if (arg == "--trace")               { _options.trace = true; continue; }
            if (arg == "--time")                { _options.time = true; continue; }
            if (arg == "--no-output")           { _options.noOutput = true; continue; }
            if (arg == "--test")                { _options.test = true; continue; }
            if (arg == "--mem")                 { _options.mem = true; continue; }
            if (arg == "--bench-mem")           { _options.benchMem = true; continue; }

            // --bench, or --bench=<runs>
            if (arg == "--bench" || arg.starts_with("--bench="))
            {
                _options.bench = true;
                if (arg != "--bench")
                {
                    std::string_view const count = arg.substr(std::string_view("--bench=").size());
                    int runs = 0;
                    auto const [end, error] = std::from_chars(count.data(), count.data() + count.size(), runs);
                    if (error != std::errc() || end != count.data() + count.size() || runs < 1)
                    {
                        _error = "invalid number of runs '" + std::string(count) + "' after --bench=";
                        return false;
                    }
                    _options.benchRuns = runs;
                }
                continue;
            }

            // --mode=<name>, or --mode <name>
            if (arg == "--mode" || arg.starts_with("--mode="))
            {
                std::string_view name;
                if (arg == "--mode")
                {
                    if (i + 1 >= _argc) { _error = "missing value after --mode"; return false; }
                    name = _argv[++i];
                }
                else
                    name = arg.substr(std::string_view("--mode=").size());

                std::optional<Mode> mode = ParseMode(name);
                if (!mode)
                {
                    _error = "unknown mode '" + std::string(name) + "', expected tree, vm or jit";
                    return false;
                }
                _options.mode = *mode;
                _options.modeSet = true;
                continue;
            }

            if (arg.size() > 1 && arg.front() == '-') { _error = "unknown option '" + std::string(arg) + "'"; return false; }
            if (hasFile) { _error = "only one source file can be given"; return false; }
            _options.file = fs::path(arg);
            hasFile = true;
        }

        if (hasFile == false && _options.help == false && _options.test == false && _options.bench == false && _options.benchMem == false) { _error = "no source file given"; return false; }
        return true;
    }

    std::string Usage(std::string_view _program)
    {
        std::string usage = "usage: ";
        usage += _program;
        usage +=
            " <file> [options]\n"
            "       ";
        usage += _program;
        usage +=
            " --test\n"
            "       ";
        usage += _program;
        usage +=
            " [file] --bench[=runs]\n"
            "       ";
        usage += _program;
        usage +=
            " [file] --bench-mem\n"
            "\n"
            "options:\n"
            "  --mode=tree|vm|jit  how the program runs (default: tree)\n"
            "  --dump-tokens       print the tokens\n"
            "  --dump-ast          print the typed AST\n"
            "  --dump-bytecode     print the bytecode (vm and jit modes)\n"
            "  --trace             trace the execution\n"
            "  --time              print the time spent in each stage\n"
            "  --no-output         drop the program output (benchmarks)\n"
            "  --bench[=runs]      time the file (default: every file of res/Lebon/benchmarks) on tree, vm and jit,\n"
            "                      or only on the --mode given; 10 timed runs by default\n"
            "  --mem               print the memory used by the run (peaks of the process, allocations of the run stage)\n"
            "  --bench-mem         memory of the file (default: every benchmark) on tree, vm and jit, or only the --mode given;\n"
            "                      each back end runs in its own process, the peaks are compared with an empty program\n"
            "  --test              run every test of res/Lebon (no file needed), exit code 1 if one fails\n"
            "  -h, --help          show this help\n";
        return usage;
    }
}