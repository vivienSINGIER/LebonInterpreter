#include "main.h"

#include "core/FileHelper.h"
#include "Driver/Benchmark.h"
#include "Driver/Options.h"
#include "Driver/StageTimes.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"
#include "Runtime/Runtime.h"
#include "Compiler/Compiler.h"
#include "Bytecode/Disassembler.h"
#include "VM/VM.h"
#include "Tree-Walking/TreeWalking.h"
#include "JIT/CodeGen.h"
#include "JIT/Jit.hpp"
#include "Test/Test.hpp"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    // Exit code of an invalid command line (EX_USAGE)
    constexpr int UsageExitCode = 64;

    using Driver::StageTimes;

    class SinkBuf : public std::streambuf
    {
    public:
        explicit SinkBuf(Runtime::OutputSink& _sink) : m_sink(_sink) {}

    protected:
        std::streamsize xsputn(char const* _text, std::streamsize _count) override
        {
            m_sink.Write(std::string_view(_text, static_cast<size_t>(_count)));
            return _count;
        }

        int_type overflow(int_type _char) override
        {
            if (traits_type::eq_int_type(_char, traits_type::eof()) == false)
            {
                char const c = traits_type::to_char_type(_char);
                m_sink.Write(std::string_view(&c, 1));
            }
            return traits_type::not_eof(_char);
        }

    private:
        Runtime::OutputSink& m_sink;
    };

    Error RunTree(Driver::Options const& _options, Program& _program, Runtime::Context& _context, StageTimes& _times)
    {
        if (_options.dumpBytecode)
            Log::Log(LogType::Warning, "--dump-bytecode has no effect in tree mode\n");

        Error result = Error::Ok();

        _times.Measure("run", [&] {
            try
            {
                RUNTIME::TreeWalking tree(*_context.out);
                tree.Run(_program);
            }
            catch (RUNTIME::RuntimeError const& e)
            {
                result = Error::Execution(e.message, e.row, e.column);
            }
        });

        return result;
    }

    Error RunVm(Driver::Options const& _options, Program& _program, Runtime::Context& _context, StageTimes& _times)
    {
        if (_options.trace)
            Log::Log(LogType::Warning, "--trace isn't supported by the VM yet\n");

        Bytecode::Heap heap;
        Bytecode::Compiler compiler(heap);
        Bytecode::CompiledProgram compiled;
        _times.Measure("compile", [&] { compiled = compiler.Compile(_program); });

        // The compiler logged why it failed
        if (!compiled)
            return Error::Ok();

        if (_options.dumpBytecode)
            Bytecode::Disassemble(*compiled.main, std::cout, compiled.globalNames);

        SinkBuf buffer(*_context.out);
        std::ostream out(&buffer);

        Bytecode::VM vm(heap);
        vm.SetOutput(out);

        // The VM logs its own runtime errors
        _times.Measure("run", [&] {
            vm.Run(compiled);
            out.flush();
        });

        return Error::Ok();
    }

    Error RunJit(Program& _program, Runtime::Context& _context, StageTimes& _times)
    {
#ifdef _M_X64
        Jit::JitCode jit;
        Jit::CodeGen codeGen(jit, _program.stack.table);

        bool generated = false;
        _times.Measure("compile", [&] { generated = codeGen.Run(_program); });

        // The code generator logged why it failed
        if (generated == false)
            return Error::Ok();

        if (jit.code.Entry() == nullptr)
            return Error::Execution("the JIT couldn't get executable memory", 0, 0);

        _times.Measure("run", [&] { jit.Run(*_context.out); });

        return Error::Ok();
#else
        (void)_program;
        (void)_context;
        (void)_times;
        return Error::Execution("the JIT only runs in a 64 bits build", 0, 0);
#endif
    }

    // Runs the analysed program with the back end chosen on the command line, then flushes its output.
    // Each back end times its own stages. Returns its error, Ok if the program ran.
    Error Execute(Driver::Options const& _options, Program& _program, Runtime::Context& _context, StageTimes& _times)
    {
        Error result = Error::Ok();

        switch (_options.mode)
        {
        case Driver::Mode::Tree: 
            result = RunTree(_options, _program, _context, _times);
            break;
        case Driver::Mode::Vm:   
            result = RunVm(_options, _program, _context, _times);   
            break;
        case Driver::Mode::Jit:  
            result = RunJit(_program, _context, _times);           
            break;
        }

        _context.out->Flush();
        return result;
    }

    // Runs the lexer, the parser, the analyser then the back end on one file, the program prints on _out.
    // A stage only runs if the previous ones logged no error.
    // Returns the code of the first error, Ok if the file went through.
    Error::ErrorCode RunFile(Driver::Options const& _options, Runtime::OutputSink& _out, StageTimes& times)
    {
        Lexer lexer(_options.file);
        times.Measure("lex", [&] { lexer.Scan(); });

        if (_options.dumpTokens)
            lexer.DisplayTokens();

        std::unique_ptr<Program> program;

        if (ErrorManager::HasErrors() == false)
        {
            times.Measure("parse", [&] {
                Parser parser(lexer.GetTokens());
                program = parser.Parse();
            });
        }

        if (program && ErrorManager::HasErrors() == false)
        {
            times.Measure("analyse", [&] {
                Semantics::Analyser analyser;
                analyser.Run(*program);
            });

            if (_options.dumpAst)
            {
                AstPrinter printer;
                printer.Print(*program);
            }
        }

        if (program && ErrorManager::HasErrors() == false)
        {
            Runtime::Context context;
            context.out = &_out;

            ErrorManager::LogError(Execute(_options, *program, context, times));
        }

        Error::ErrorCode code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return code;
    }

    // The command line run: the output goes to the console unless --no-output, --time prints the stages
    Error::ErrorCode RunCommandLine(Driver::Options const& _options)
    {
        Runtime::ConsoleSink console;
        Runtime::NullSink null;
        StageTimes times;

        Error::ErrorCode const code = RunFile(_options, _options.noOutput ? static_cast<Runtime::OutputSink&>(null) : console, times);

        if (_options.time)
            times.Print();

        if (_options.mem)
            std::fprintf(stderr, "%s\n", Driver::FormatMemoryLine(Driver::Memory::Process(), times.MemoryOf("run")).c_str());

        return code;
    }

    // The file given, or every program of res/Lebon/benchmarks. False if there is none
    bool BenchmarkFiles(Driver::Options const& _options, std::vector<fs::path>& _files)
    {
        if (_options.file.empty() == false)
        {
            _files.push_back(_options.file);
            return true;
        }

        fs::path tests;
        if (Error e = Test::FindTestsDir(tests))
        {
            Log::Log(LogType::Error, e.Format() + "\n");
            return false;
        }

        fs::path const folder = tests.parent_path() / "benchmarks";
        _files = Test::LbnFilesIn(folder);
        if (_files.empty())
        {
            Log::Log(LogType::Error, "no benchmark found in " + folder.string() + "\n");
            return false;
        }
        return true;
    }

    // --bench: time every file on each back end. Returns the exit code
    int RunBench(Driver::Options _options)
    {
        std::vector<fs::path> files;
        if (BenchmarkFiles(_options, files) == false)
            return 1;

        // Dumps and timings would drown the table
        _options.dumpTokens = _options.dumpAst = _options.dumpBytecode = _options.trace = _options.time = false;

        return Driver::RunBenchmark(_options, files, RunFile) == 0 ? 0 : 1;
    }

    // --bench-mem: the memory of every file on each back end. Returns the exit code
    int RunBenchMem(Driver::Options const& _options)
    {
        std::vector<fs::path> files;
        if (BenchmarkFiles(_options, files) == false)
            return 1;

        return Driver::RunMemoryBenchmark(_options, files) == 0 ? 0 : 1;
    }
}

int main(int _argc, char** _argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string const program = _argc > 0 ? fs::path(_argv[0]).stem().string() : "lebon";

    Driver::Options options;
    std::string error;
    if (Driver::ParseCommandLine(_argc, _argv, options, error) == false)
    {
        Log::Log(LogType::Error, program + ": " + error + "\n");
        std::fputs(Driver::Usage(program).c_str(), stderr);
        return UsageExitCode;
    }

    if (options.help)
    {
        std::fputs(Driver::Usage(program).c_str(), stdout);
        return 0;
    }

    if (options.test)
        return Test::RunAllTests() == 0 ? 0 : 1;

    if (options.bench)
        return RunBench(options);

    if (options.benchMem)
        return RunBenchMem(options);

    // The exit code is the code of the first error: 0 ok, 1 lexical, 2 syntax,
    // 3 semantics, 4 execution, 5 io
    return static_cast<int>(RunCommandLine(options));
}