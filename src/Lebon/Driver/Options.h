#ifndef DRIVER_OPTIONS_H_DEFINED
#define DRIVER_OPTIONS_H_DEFINED

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Driver
{
    enum class Mode
    {
        Tree, // tree-walking interpreter
        Vm,   // bytecode virtual machine
        Jit   // native code
    };

    struct Options
    {
        fs::path file;
        Mode mode = Mode::Tree;
        bool modeSet = false;       // --mode was given

        bool dumpTokens = false;
        bool dumpAst = false;
        bool dumpBytecode = false;
        bool trace = false;
        bool time = false;
        bool noOutput = false;
        bool test = false;
        bool mem = false;           // print the memory used by the run on stderr
        bool bench = false;
        bool benchMem = false;
        int benchRuns = 10;
        bool help = false;
    };

    char const* ModeName(Mode _mode);

    // Reads argv into _options. Returns false and fills _error when the command line is invalid
    bool ParseCommandLine(int _argc, char** _argv, Options& _options, std::string& _error);

    std::string Usage(std::string_view _program);
}

#endif