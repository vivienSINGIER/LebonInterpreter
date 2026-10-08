#include "main.h"

#include "core/FileHelper.h"
#include "Driver/Options.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"
#include "runtime/Runtime.h"
#include "Compiler/Compiler.h"
#include "Bytecode/Disassembler.h"
#include "VM/VM.h"
#include "Test/Test.hpp"
#include "Tree-Walking/TreeWalking.h"

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

    // Time spent in each stage, printed on stderr with --time
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

        void Print() const
        {
            double total = 0;
            for (auto const& [stage, ms] : m_stages)
            {
                std::fprintf(stderr, "[time] %-8s %10.3f ms\n", stage, ms);
                total += ms;
            }
            std::fprintf(stderr, "[time] %-8s %10.3f ms\n", "total", total);
        }

    private:
        std::vector<std::pair<char const*, double>> m_stages;
    };

    // The VM prints on a std::ostream, this sends what it writes to the output sink of the run
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

    // Compiles the analysed program to bytecode then runs it on the VM.
    // The compilation and the execution are timed apart, like the other stages.
    void RunVm(Driver::Options const& _options, Program& _program, Runtime::Context& _context, StageTimes& _times)
    {
        if (_options.trace)
            Log::Log(LogType::Warning, "--trace isn't supported by the VM yet\n");

        Bytecode::Heap heap;
        Bytecode::Compiler compiler(heap);
        Bytecode::CompiledProgram compiled;
        _times.Measure("compile", [&] { compiled = compiler.Compile(_program); });

        // The compiler logged why it failed
        if (!compiled)
            return;

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
            _context.out->Flush();
        });
    }

    // Runs the analysed program with the back end chosen on the command line
    Error Execute(Driver::Options const& _options, Program& _program, Runtime::Context& _context)
    {
        switch (_options.mode)
        {
        case Driver::Mode::Tree:
            try
            {
                RUNTIME::TreeWalking tree(*_context.out);
                tree.Run(_program);
            }
            catch (RUNTIME::RuntimeError const& e)
            {
                return Error::Execution(e.message, e.row, e.column);
            }
            return Error::Ok();
        case Driver::Mode::Vm:   return Error::Ok(); // RunVm qui gère l'exécution du VM
        case Driver::Mode::Jit:  return Error::Execution("the JIT isn't implemented yet", 0, 0);
        }
        return Error::Ok();
    }

    // Runs the lexer, the parser, the analyser then the back end on one file.
    // A stage only runs if the previous ones logged no error.
    // Returns the code of the first error, Ok if the file went through.
    Error::ErrorCode RunFile(Driver::Options const& _options)
    {
        StageTimes times;

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

            // Printed after the analysis so the types and symbol ids are filled
            if (_options.dumpAst)
            {
                AstPrinter printer;
                printer.Print(*program);
            }
        }

        if (program && ErrorManager::HasErrors() == false)
        {
            if (_options.dumpBytecode && _options.mode == Driver::Mode::Tree)
                Log::Log(LogType::Warning, "--dump-bytecode has no effect in tree mode\n");

            Runtime::ConsoleSink console;
            Runtime::NullSink null;

            Runtime::Context context;
            context.out = _options.noOutput ? static_cast<Runtime::OutputSink*>(&null) : &console;

            if (_options.mode == Driver::Mode::Vm)
            {
                RunVm(_options, *program, context, times);
            }
            else
            {
                times.Measure("run", [&] {
                    ErrorManager::LogError(Execute(_options, *program, context));
                    context.out->Flush();
                });
            }
        }

        if (_options.time)
            times.Print();

        Error::ErrorCode code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return code;
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

    // The exit code is the code of the first error: 0 ok, 1 lexical, 2 syntax,
    // 3 semantics, 4 execution, 5 io
    return static_cast<int>(RunFile(options));
}