#include "main.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <vector>

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"
#include "Bytecode/BytecodeTests.h"
#include "Bytecode/Disassembler.h"
#include "Compiler/Compiler.h"

#include <windows.h>

namespace
{
    char const* StageName(Error::ErrorCode _code)
    {
        switch (_code)
        {
        case Error::ErrorCode::Ok:        return "no error";
        case Error::ErrorCode::Lexical:   return "lexical error";
        case Error::ErrorCode::Syntax:    return "syntax error";
        case Error::ErrorCode::Semantics: return "semantics error";
        case Error::ErrorCode::Execution: return "execution error";
        case Error::ErrorCode::Io:        return "io error";
        }
        return "error";
    }

    std::string ReadText(fs::path const& _path, bool& _found)
    {
        std::ifstream in(_path, std::ios::binary);
        _found = in.good();

        std::stringstream content;
        content << in.rdbuf();

        // A checkout on Windows may turn \n into \r\n
        std::string text = content.str();
        std::erase(text, '\r');
        return text;
    }

    // Compiles the program and compares the disassembly with the .asm next to the test.
    // A difference is logged as an error, so the test fails
    void CompileAndCheck(Program& _program, fs::path const& _path)
    {
        Bytecode::Heap heap;
        Bytecode::Compiler compiler(heap);
        std::unique_ptr<Bytecode::Prototype> script = compiler.Compile(_program);
        if (script == nullptr)
            return;

        std::ostringstream got;
        Bytecode::Disassemble(*script, got);

        fs::path asmPath = fs::path(_path).replace_extension(".asm");
        bool found = false;
        std::string expected = ReadText(asmPath, found);

        if (found == false)
        {
            ErrorManager::LogError(Error::Io("missing expected bytecode", asmPath));
            return;
        }

        if (got.str() != expected)
        {
            ErrorManager::LogError(Error::Execution("bytecode differs from " + asmPath.filename().string(), 0, 0));
            Log::Log(LogType::Warning, "got :\n" + got.str());
        }
    }

    // Runs the lexer, the parser and the analyser on one file.
    // A stage only runs if the previous ones logged no error.
    // The files of the "compile" folder also go through the compiler.
    // Returns the code of the first error, Ok if the file went through.
    Error::ErrorCode RunFile(fs::path const& _path, bool _verbose)
    {
        Lexer lexer(_path);
        lexer.Scan();
        if (_verbose)
            lexer.DisplayTokens();

        if (ErrorManager::HasErrors() == false)
        {
            Parser parser(lexer.GetTokens());
            std::unique_ptr<Program> program = parser.Parse();

            if (program && ErrorManager::HasErrors() == false)
            {
                if (_verbose)
                {
                    AstPrinter printer;
                    printer.Print(*program);
                }

                Semantics::Analyser analyser;
                analyser.Run(*program);

                if (ErrorManager::HasErrors() == false && _path.parent_path().filename() == "compile")
                    CompileAndCheck(*program, _path);
            }
        }

        Error::ErrorCode code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return code;
    }

    // The folder of a test names the stage that has to reject it
    Error::ErrorCode Expected(fs::path const& _path)
    {
        std::string stage = _path.parent_path().filename().string();

        if (stage == "lexing")    return Error::ErrorCode::Lexical;
        if (stage == "parsing")   return Error::ErrorCode::Syntax;
        if (stage == "semantics") return Error::ErrorCode::Semantics;
        return Error::ErrorCode::Ok;
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    if (Bytecode::RunSelfTests() == false)
        return 1;

    // Set to true to also print the tokens and the AST of every file
    bool const verbose = false;

    std::vector<fs::path> files;
    if (Error e = FileHelper::ListDir(fs::path("../../res/Lebon/tests"), files))
    {
        ErrorManager::LogError(e);
        return e.Exit();
    }

    uint32_t failed = 0;
    for (fs::path const& file : files)
    {
        if (FileHelper::ExtLower(file) != ".lbn")
            continue;

        std::string name = file.parent_path().filename().string() + "/" + file.filename().string();
        Log::Log(LogType::Prompt, "==== " + name + " ====\n");
        std::cout.flush();

        Error::ErrorCode expected = Expected(file);
        Error::ErrorCode got = RunFile(file, verbose);

        if (got == expected)
        {
            Log::Log(LogType::Info, std::string("[PASS] ") + StageName(got) + "\n\n");
        }
        else
        {
            Log::Log(LogType::Warning, std::string("[FAIL] expected ") + StageName(expected) + ", got " + StageName(got) + "\n\n");
            failed++;
        }
        std::cout.flush();
    }

    return failed == 0 ? 0 : 1;
}
