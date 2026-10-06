#include "main.h"

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Semantics/Analyser.h"
#include "Compiler/Compiler.h"
#include "Bytecode/BytecodeTests.h"
#include "Bytecode/Disassembler.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include <windows.h>

namespace
{
    struct Outcome
    {
        Error::ErrorCode code = Error::ErrorCode::Ok;
        std::string firstError;      // the first error logged, empty if none
        std::string disassembly;     // only filled when the file was compiled
    };

    // Lance le lexer, le parser, l'analyseur et le compilateur sur un fichier.
    // Une étape ne tourne que si les précédentes n'ont loggé aucune erreur.
    // Renvoie le code de la première erreur, Ok si le fichier est passé partout.
    Outcome RunPipeline(fs::path const& _path, bool _verbose)
    {
        Outcome outcome;

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
                Semantics::Analyser analyser;
                analyser.Run(*program);

                // Printed after the analysis so the types and symbol ids are filled
                if (_verbose)
                {
                    //AstPrinter printer;
                    //printer.Print(*program);
                }
                                         
                if (ErrorManager::HasErrors() == false)
                {
                    Bytecode::Heap heap;
                    Bytecode::Compiler compiler(heap);
                    std::unique_ptr<Bytecode::Prototype> main = compiler.Compile(*program);

                    if (main)
                    {
                        std::ostringstream text;
                        Bytecode::Disassemble(*main, text);
                        outcome.disassembly = text.str();

                        if (_verbose)
                            std::cout << outcome.disassembly;
                    }
                }
            }
        }

        if (ErrorManager::HasErrors())
            outcome.firstError = ErrorManager::Errors().front().Format();

        outcome.code = static_cast<Error::ErrorCode>(ErrorManager::Code());
        ErrorManager::Clear();

        return outcome;
    }

    // Les fichiers de test loggent des erreurs exprès : tant que l'objet vit, cout et cerr sont redirigés vers un tampon
    class Silencer
    {
    public:
        Silencer()
            : m_out(std::cout.rdbuf(m_sink.rdbuf()))
            , m_err(std::cerr.rdbuf(m_sink.rdbuf()))
        {}

        ~Silencer()
        {
            std::cout.rdbuf(m_out);
            std::cerr.rdbuf(m_err);
        }

    private:
        std::ostringstream m_sink;
        std::streambuf* m_out;
        std::streambuf* m_err;
    };

    // Nom lisible d'un code d'erreur, pour les messages de test
    char const* CodeName(Error::ErrorCode _code)
    {
        switch (_code)
        {
        case Error::ErrorCode::Ok:         return "ok";
        case Error::ErrorCode::Lexical:    return "lexical error";
        case Error::ErrorCode::Syntax:     return "syntax error";
        case Error::ErrorCode::Semantics:  return "semantics error";
        case Error::ErrorCode::Execution:  return "execution error";
        case Error::ErrorCode::Io:         return "io error";
        }
        return "unknown";
    }

    // Retire les \r et les blancs de fin pour qu'ils ne fassent pas échouer la comparaison d'un listing
    std::string Normalize(std::string _text)
    {
        _text.erase(std::remove(_text.begin(), _text.end(), '\r'), _text.end());
        while (_text.empty() == false && (_text.back() == '\n' || _text.back() == ' '))
            _text.pop_back();
        return _text;
    }

    struct TestStats
    {
        int passed = 0;
        int failed = 0;

        // Compte et affiche un test (vert si réussi, rouge avec le détail sinon)
        void Report(std::string const& _name, bool _ok, std::string const& _detail = "")
        {
            if (_ok)
            {
                passed++;
                Log::Log(LogType::Info, "  [pass] " + _name + "\n");
                return;
            }

            failed++;
            Log::Log(LogType::Error, "  [FAIL] " + _name + "\n");
            if (_detail.empty() == false)
                Log::Log(LogType::Error, _detail + "\n");
        }
    };

    // Les fichiers .lbn directement dans le dossier (sans les sous-dossiers), triés par nom
    std::vector<fs::path> LbnFilesIn(fs::path const& _dir)
    {
        std::vector<fs::path> all, files;
        FileHelper::ListDir(_dir, all);

        for (fs::path const& path : all)
        {
            if (path.parent_path() == _dir && FileHelper::ExtLower(path) == ".lbn")
                files.push_back(path);
        }
        return files;
    }

    // Chaque fichier du dossier doit s'arrêter sur le type d'erreur attendu (Ok pour les fichiers valides)
    void TestFolder(fs::path const& _root, char const* _folder, Error::ErrorCode _expected, TestStats& _stats)
    {
        Log::Log(LogType::PromptInfo, std::string("[") + _folder + "] expects " + CodeName(_expected) + "\n");

        std::vector<fs::path> files = LbnFilesIn(_root / _folder);
        _stats.Report(std::string(_folder) + " folder is not empty", files.empty() == false);

        for (fs::path const& file : files)
        {
            Outcome outcome;
            {
                Silencer silence;
                outcome = RunPipeline(file, false);
            }

            std::string detail;
            if (outcome.code != _expected)
                detail = std::string("    expected ") + CodeName(_expected) + ", got " + CodeName(outcome.code) + (outcome.firstError.empty() ? "" : " : " + outcome.firstError);

            _stats.Report(std::string(_folder) + "/" + file.filename().string(), outcome.code == _expected, detail);
        }
    }

    // Chaque fichier doit se compiler sans erreur en exactement le listing .asm placé à côté
    void TestCompiler(fs::path const& _root, TestStats& _stats)
    {
        Log::Log(LogType::PromptInfo, "[compiler] bytecode listings\n");

        std::vector<fs::path> files = LbnFilesIn(_root / "compiler");
        _stats.Report("compiler folder is not empty", files.empty() == false);

        for (fs::path const& file : files)
        {
            std::string name = "compiler/" + file.filename().string();

            Outcome outcome;
            {
                Silencer silence;
                outcome = RunPipeline(file, false);
            }

            if (outcome.code != Error::ErrorCode::Ok)
            {
                _stats.Report(name, false, std::string("    ") + CodeName(outcome.code) + " : " + outcome.firstError);
                continue;
            }

            fs::path listing = file;
            listing.replace_extension(".asm");

            std::string expected;
            if (Error e = FileHelper::ReadFile(listing, expected))
            {
                _stats.Report(name, false, "    " + e.Format());
                continue;
            }

            bool same = Normalize(expected) == Normalize(outcome.disassembly);
            _stats.Report(name, same, same ? "" : "    got:\n" + outcome.disassembly + "    expected:\n" + expected);
        }
    }

    // Auto-tests du bytecode, puis tous les dossiers de res/Lebon/tests. Renvoie le nombre de tests en échec
    int RunAllTests()
    {
        TestStats stats;

        Log::Log(LogType::PromptInfo, "[bytecode] self tests\n");
        stats.Report("bytecode self tests", Bytecode::RunSelfTests());

        fs::path root;
        if (Error e = FileHelper::FindUpwards(fs::current_path(), "res/Lebon/tests", root))
        {
            stats.Report("tests folder found", false, "    " + e.Format());
        }
        else
        {
            fs::path tests = root / "res" / "Lebon" / "tests";

            TestFolder(tests, "valid",     Error::ErrorCode::Ok,        stats);
            TestFolder(tests, "lexing",    Error::ErrorCode::Lexical,   stats);
            TestFolder(tests, "parsing",   Error::ErrorCode::Syntax,    stats);
            TestFolder(tests, "semantics", Error::ErrorCode::Semantics, stats);
            TestCompiler(tests, stats);
        }

        std::string summary = std::to_string(stats.passed) + " passed, " + std::to_string(stats.failed) + " failed\n";
        Log::Log(stats.failed == 0 ? LogType::Info : LogType::Error, summary);

        return stats.failed;
    }
}

// Sans argument   : les tests, puis le programme de démo avec les tokens, l'AST et le bytecode
// --tests         : les tests seulement
// <fichier>       : ce fichier avec les tokens, l'AST et le bytecode
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    std::string arg = argc > 1 ? argv[1] : "";

    if (arg.empty() == false && arg != "--tests")
        return RunPipeline(fs::path(arg), true).code == Error::ErrorCode::Ok ? 0 : 1;

    int failures = RunAllTests();
    if (arg == "--tests")
        return failures == 0 ? 0 : 1;

    // Le second argument à true affiche aussi les tokens, l'AST et le bytecode
    Log::Log(LogType::PromptInfo, "\n[demo] valid/program.lbn\n");
    Error::ErrorCode code = RunPipeline("../../res/Lebon/tests/valid/program.lbn", true).code;

    return failures == 0 && code == Error::ErrorCode::Ok ? 0 : 1;
}
