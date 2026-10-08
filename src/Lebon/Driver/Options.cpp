#include "Options.h"

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
                continue;
            }

            if (arg.size() > 1 && arg.front() == '-') { _error = "unknown option '" + std::string(arg) + "'"; return false; }
            if (hasFile) { _error = "only one source file can be given"; return false; }
            _options.file = fs::path(arg);
            hasFile = true;
        }

        if (hasFile == false && _options.help == false) { _error = "no source file given"; return false; }
        return true;
    }

    std::string Usage(std::string_view _program)
    {
        std::string usage = "usage: ";
        usage += _program;
        usage +=
            " <file> [options]\n"
            "\n"
            "options:\n"
            "  --mode=tree|vm|jit  how the program runs (default: tree)\n"
            "  --dump-tokens       print the tokens\n"
            "  --dump-ast          print the typed AST\n"
            "  --dump-bytecode     print the bytecode (vm and jit modes)\n"
            "  --trace             trace the execution\n"
            "  --time              print the time spent in each stage\n"
            "  --no-output         drop the program output (benchmarks)\n"
            "  -h, --help          show this help\n";
        return usage;
    }
}