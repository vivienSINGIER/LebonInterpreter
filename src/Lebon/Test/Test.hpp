#ifndef TEST_HPP_DEFINED
#define TEST_HPP_DEFINED

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
#include "VM/VM.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace Test
{
    struct Outcome
    {
        Error::ErrorCode code = Error::ErrorCode::Ok;
        std::string firstError;      // la première erreur loggée, vide s'il n'y en a pas
        std::string disassembly;     // rempli seulement si le fichier a été compilé
        std::string globalsError;    // problème dans la table des globales du résultat de compilation, vide si tout va bien
        std::string output;          // ce qu'afise a écrit, rempli seulement si le programme a été exécuté
    };

    // Chaque GETGLOBAL / SETGLOBAL de la fonction et de ses fonctions internes doit viser un slot de la table
    inline bool GlobalSlotsInRange(Bytecode::Prototype const& _proto, size_t _count)
    {
        for (Bytecode::Instruction i : _proto.code)
        {
            Bytecode::OpCode op = Bytecode::GetOp(i);
            if ((op == Bytecode::OpCode::GetGlobal || op == Bytecode::OpCode::SetGlobal) && Bytecode::GetBx(i) >= _count)
                return false;
        }

        for (auto const& child : _proto.protos)
        {
            if (GlobalSlotsInRange(*child, _count) == false)
                return false;
        }
        return true;
    }

    // Lance le lexer, le parser, l'analyseur et le compilateur sur un fichier, puis la VM si _run est vrai.
    // Une étape ne tourne que si les précédentes n'ont loggé aucune erreur.
    // Renvoie le code de la première erreur, Ok si le fichier est passé partout.
    inline Outcome RunPipeline(fs::path const& _path, bool _verbose, bool _run = false)
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

                // Affiché après l'analyse pour que les types et les ids de symboles soient remplis
                if (_verbose)
                {
                    AstPrinter printer;
                    printer.Print(*program);
                }

                if (ErrorManager::HasErrors() == false)
                {
                    Bytecode::Heap heap;
                    Bytecode::Compiler compiler(heap);
                    Bytecode::CompiledProgram compiled = compiler.Compile(*program);

                    if (compiled)
                    {
                        // Les noms des globales servent à commenter GETGLOBAL / SETGLOBAL dans le listing
                        std::ostringstream text;
                        Bytecode::Disassemble(*compiled.main, text, compiled.globalNames);
                        outcome.disassembly = text.str();

                        if (compiled.FindGlobal("afise") == Bytecode::CompiledProgram::NoGlobal)
                            outcome.globalsError = "the built-in 'afise' has no global slot";
                        else if (GlobalSlotsInRange(*compiled.main, compiled.GlobalCount()) == false)
                            outcome.globalsError = "an instruction uses a global slot outside of the table (" + std::to_string(compiled.GlobalCount()) + " globals)";

                        if (_verbose)
                            std::cout << outcome.disassembly;

                        // La sortie d'afise est récupérée dans un flux : les tests la comparent, le mode verbeux l'affiche
                        if (_run && outcome.globalsError.empty())
                        {
                            std::ostringstream captured;
                            Bytecode::VM vm(heap);
                            vm.SetOutput(captured);
                            vm.Run(compiled);

                            outcome.output = captured.str();
                            if (_verbose)
                                std::cout << "\n== run ==\n" << outcome.output;
                        }
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

    // Pipeline complet sur un fichier, exécution comprise, renvoie seulement le code d'erreur
    inline Error::ErrorCode RunFile(fs::path const& _path, bool _verbose)
    {
        return RunPipeline(_path, _verbose, true).code;
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
    inline char const* CodeName(Error::ErrorCode _code)
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
    inline std::string Normalize(std::string _text)
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
    inline std::vector<fs::path> LbnFilesIn(fs::path const& _dir)
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

    // Chaque fichier du dossier doit s'arrêter sur le type d'erreur attendu (Ok pour les fichiers valides).
    // Un fichier compilé doit aussi avoir une table de globales cohérente (ce qui ne concerne que les fichiers valides).
    // _run : les fichiers sont aussi exécutés, pour les erreurs qui n'arrivent qu'à l'exécution
    inline void TestFolder(fs::path const& _root, char const* _folder, Error::ErrorCode _expected, TestStats& _stats, bool _run = false)
    {
        Log::Log(LogType::PromptInfo, std::string("[") + _folder + "] expects " + CodeName(_expected) + "\n");

        for (fs::path const& file : LbnFilesIn(_root / _folder))
        {
            Outcome outcome;
            {
                Silencer silence;
                outcome = RunPipeline(file, false, _run);
            }

            std::string detail;
            if (outcome.code != _expected)
                detail = std::string("    expected ") + CodeName(_expected) + ", got " + CodeName(outcome.code) + (outcome.firstError.empty() ? "" : " : " + outcome.firstError);
            else if (outcome.globalsError.empty() == false)
                detail = "    " + outcome.globalsError;

            _stats.Report(std::string(_folder) + "/" + file.filename().string(), detail.empty(), detail);
        }
    }

    // Chaque fichier doit se compiler sans erreur en exactement le listing .asm placé à côté
    inline void TestCompiler(fs::path const& _root, TestStats& _stats)
    {
        Log::Log(LogType::PromptInfo, "[compiler] bytecode listings\n");

        for (fs::path const& file : LbnFilesIn(_root / "compiler"))
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

            // Le listing contient déjà les slots et les noms des globales, il n'y a rien de plus à vérifier sur la table
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

    // Chaque programme du dossier est exécuté sans erreur et doit écrire exactement ce qu'il y a dans le fichier .out placé à côté
    inline void TestOutputs(fs::path const& _dir, std::string const& _label, TestStats& _stats)
    {
        Log::Log(LogType::PromptInfo, "[" + _label + "] program output\n");

        for (fs::path const& file : LbnFilesIn(_dir))
        {
            std::string name = _label + "/" + file.filename().string();

            Outcome outcome;
            {
                Silencer silence;
                outcome = RunPipeline(file, false, true);
            }

            if (outcome.code != Error::ErrorCode::Ok)
            {
                _stats.Report(name, false, std::string("    ") + CodeName(outcome.code) + " : " + outcome.firstError);
                continue;
            }

            fs::path expectedFile = file;
            expectedFile.replace_extension(".out");

            std::string expected;
            if (Error e = FileHelper::ReadFile(expectedFile, expected))
            {
                _stats.Report(name, false, "    " + e.Format());
                continue;
            }

            bool same = Normalize(expected) == Normalize(outcome.output);
            _stats.Report(name, same, same ? "" : "    got:\n" + outcome.output + "    expected:\n" + expected);
        }
    }

    // Dossier res/Lebon/tests, cherché en remontant depuis le dossier courant
    inline Error FindTestsDir(fs::path& _out)
    {
        fs::path root;
        if (Error e = FileHelper::FindUpwards(fs::current_path(), "res/Lebon/tests", root))
            return e;

        _out = root / "res" / "Lebon" / "tests";
        return Error::Ok();
    }

    // Auto-tests du bytecode, puis tous les dossiers de res/Lebon/tests. Renvoie le nombre de tests en échec
    inline int RunAllTests()
    {
        TestStats stats;

        Log::Log(LogType::PromptInfo, "[bytecode] self tests\n");
        stats.Report("bytecode self tests", Bytecode::RunSelfTests());

        fs::path tests;
        if (Error e = FindTestsDir(tests))
        {
            stats.Report("tests folder found", false, "    " + e.Format());
        }
        else
        {
            // Un dossier vide ou introuvable ne ferait échouer aucun test : on le vérifie une seule fois pour tous
            bool allFilled = true;
            for (char const* folder : { "valid", "lexing", "parsing", "semantics", "compiler", "vm", "runtime" })
                allFilled = allFilled && LbnFilesIn(tests / folder).empty() == false;
            allFilled = allFilled && LbnFilesIn(tests.parent_path() / "benchmarks").empty() == false;
            stats.Report("every test folder has files", allFilled);

            TestFolder(tests, "valid",     Error::ErrorCode::Ok,        stats);
            TestFolder(tests, "lexing",    Error::ErrorCode::Lexical,   stats);
            TestFolder(tests, "parsing",   Error::ErrorCode::Syntax,    stats);
            TestFolder(tests, "semantics", Error::ErrorCode::Semantics, stats);
            TestCompiler(tests, stats);
            TestOutputs(tests / "vm", "vm", stats);
            TestFolder(tests, "runtime",   Error::ErrorCode::Execution, stats, true);

            // Les benchmarks sont aussi des programmes dont le résultat doit rester juste après une optimisation.
            // Ici une seule exécution sans mesure, les mesures sont dans --bench
            TestOutputs(tests.parent_path() / "benchmarks", "benchmarks", stats);
        }

        std::string summary = std::to_string(stats.passed) + " passed, " + std::to_string(stats.failed) + " failed\n";
        Log::Log(stats.failed == 0 ? LogType::Info : LogType::Error, summary);

        return stats.failed;
    }
}

#endif
