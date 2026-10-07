#ifndef BENCHMARK_HPP_DEFINED
#define BENCHMARK_HPP_DEFINED

#include "Test/Test.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

// Benchmarks : des programmes Lebon de res/Lebon/benchmarks, chacun avec son .out.
// Ils servent à mesurer l'effet d'une optimisation du compilateur ou de la VM, et à vérifier qu'elle ne change pas le résultat.
//
//   Le nombre d'instructions du bytecode       (compilateur, exact et identique d'une machine à l'autre)
//   Le nombre d'instructions exécutées         (compilateur, exact : c'est ce que réduisent les optimisations du bytecode)
//   Le temps d'exécution de la VM              (mesuré, meilleur et médiane sur plusieurs exécutions)
//   Les objets et chaînes créés par exécution  (mémoire : il n'y a pas de ramasse-miettes)
namespace Test
{
    struct BenchResult
    {
        std::string name;
        std::string error;                  // vide si tout s'est bien passé
        size_t staticInstructions = 0;      // instructions du bytecode, toutes fonctions comprises
        uint64_t executed = 0;              // instructions exécutées par un run
        std::vector<std::pair<Bytecode::OpCode, uint64_t>> opCounts;   // par opcode, du plus fréquent au plus rare
        double compileMs = 0;
        double minMs = 0;
        double medianMs = 0;
        size_t objectsPerRun = 0;           // objets créés par la première exécution
        size_t stringsPerRun = 0;           // chaînes nouvelles internées par la première exécution
    };

    // Nombre d'instructions de la fonction et de toutes ses fonctions internes
    inline size_t CountStaticInstructions(Bytecode::Prototype const& _proto)
    {
        size_t count = _proto.code.size();
        for (auto const& child : _proto.protos)
            count += CountStaticInstructions(*child);
        return count;
    }

    // Compile un fichier jusqu'au bytecode. Renvoie false et la première erreur si une étape échoue
    inline bool BuildProgram(fs::path const& _path, Bytecode::Heap& _heap, Bytecode::CompiledProgram& _out, std::string& _error)
    {
        bool built = false;

        Lexer lexer(_path);
        lexer.Scan();
        if (ErrorManager::HasErrors() == false)
        {
            Parser parser(lexer.GetTokens());
            std::unique_ptr<Program> program = parser.Parse();

            if (program && ErrorManager::HasErrors() == false)
            {
                Semantics::Analyser analyser;
                analyser.Run(*program);

                if (ErrorManager::HasErrors() == false)
                {
                    Bytecode::Compiler compiler(_heap);
                    _out = compiler.Compile(*program);
                    built = static_cast<bool>(_out);
                }
            }
        }

        if (ErrorManager::HasErrors())
            _error = ErrorManager::Errors().front().Format();
        ErrorManager::Clear();

        return built;
    }

    inline double ElapsedMs(std::chrono::steady_clock::time_point _from, std::chrono::steady_clock::time_point _to)
    {
        return std::chrono::duration<double, std::milli>(_to - _from).count();
    }

    // Mesure un benchmark : une première exécution avec les compteurs (sortie vérifiée, instructions, mémoire),
    // puis _repeats exécutions chronométrées sans compteurs
    inline BenchResult MeasureBenchmark(fs::path const& _path, int _repeats)
    {
        BenchResult result;
        result.name = _path.stem().string();

        Silencer silence;   // les erreurs d'un benchmark cassé sont rapportées dans result.error, pas sur la console

        Bytecode::Heap heap;
        Bytecode::CompiledProgram program;

        auto compileStart = std::chrono::steady_clock::now();
        bool built = BuildProgram(_path, heap, program, result.error);
        result.compileMs = ElapsedMs(compileStart, std::chrono::steady_clock::now());

        if (built == false)
        {
            if (result.error.empty())
                result.error = "compilation failed";
            return result;
        }

        result.staticInstructions = CountStaticInstructions(*program.main);

        std::string expected;
        fs::path expectedFile = _path;
        expectedFile.replace_extension(".out");
        if (Error e = FileHelper::ReadFile(expectedFile, expected))
        {
            result.error = e.Format();
            return result;
        }

        std::ostringstream output;
        Bytecode::VM vm(heap);
        vm.SetOutput(output);

        // Première exécution : avec les compteurs
        vm.EnableStats(true);
        size_t objectsBefore = heap.ObjectCount();
        size_t stringsBefore = heap.StringCount();

        bool ok = vm.Run(program);
        if (ok == false)
        {
            result.error = ErrorManager::HasErrors() ? ErrorManager::Errors().front().Format() : "runtime error";
            ErrorManager::Clear();
            return result;
        }

        result.objectsPerRun = heap.ObjectCount() - objectsBefore;
        result.stringsPerRun = heap.StringCount() - stringsBefore;
        result.executed = vm.InstructionsExecuted();

        for (size_t op = 0; op < static_cast<size_t>(Bytecode::OpCode::Count); op++)
        {
            Bytecode::OpCode code = static_cast<Bytecode::OpCode>(op);
            if (vm.InstructionsExecuted(code) > 0)
                result.opCounts.push_back({ code, vm.InstructionsExecuted(code) });
        }
        std::sort(result.opCounts.begin(), result.opCounts.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        if (Normalize(output.str()) != Normalize(expected))
        {
            result.error = "wrong output, got:\n" + output.str() + "expected:\n" + expected;
            return result;
        }

        // Exécutions chronométrées : sans compteurs, la sortie est jetée à chaque tour
        vm.EnableStats(false);
        std::vector<double> times;
        for (int run = 0; run < _repeats; run++)
        {
            output.str("");

            auto start = std::chrono::steady_clock::now();
            vm.Run(program);
            times.push_back(ElapsedMs(start, std::chrono::steady_clock::now()));
        }

        if (times.empty() == false)
        {
            std::sort(times.begin(), times.end());
            result.minMs = times.front();
            result.medianMs = times[times.size() / 2];
        }
        return result;
    }

    // "1234567" -> "1 234 567", pour lire les grands nombres
    inline std::string Grouped(uint64_t _value)
    {
        std::string digits = std::to_string(_value);
        std::string out;
        for (size_t i = 0; i < digits.size(); i++)
        {
            if (i > 0 && (digits.size() - i) % 3 == 0)
                out += ' ';
            out += digits[i];
        }
        return out;
    }

    // Les trois opcodes les plus exécutés avec leur part, par exemple "CALL 21%  MOVE 18%  LT 12%"
    inline std::string TopOpcodes(BenchResult const& _result, size_t _count)
    {
        std::ostringstream text;
        for (size_t i = 0; i < _result.opCounts.size() && i < _count; i++)
        {
            double share = _result.executed == 0 ? 0.0 : 100.0 * static_cast<double>(_result.opCounts[i].second) / static_cast<double>(_result.executed);
            if (i > 0)
                text << "  ";
            text << Bytecode::GetOpInfo(_result.opCounts[i].first).name << " " << static_cast<int>(share + 0.5) << "%";
        }
        return text.str();
    }

    // Lance tous les benchmarks et affiche un tableau. Renvoie le nombre de benchmarks en échec
    inline int RunBenchmarks(int _repeats)
    {
        fs::path tests;
        if (Error e = FindTestsDir(tests))
        {
            Log::Log(LogType::Error, e.Format() + "\n");
            return 1;
        }

        fs::path folder = tests.parent_path() / "benchmarks";
        std::vector<fs::path> files = LbnFilesIn(folder);
        if (files.empty())
        {
            Log::Log(LogType::Error, "no benchmark found in " + folder.string() + "\n");
            return 1;
        }

#ifndef NDEBUG
        Log::Log(LogType::Warning, "debug build : the VM checks the types of every operation, the times are not representative.\n"
                                   "The instruction counts do not depend on the build.\n\n");
#endif
        Log::Log(LogType::PromptInfo, "[benchmarks] " + std::to_string(_repeats) + " timed run(s) each, VM time only\n");

        std::ostringstream header;
        header << std::left << std::setw(13) << "benchmark"
               << std::right << std::setw(8) << "code"
               << std::setw(15) << "executed"
               << std::setw(10) << "min ms"
               << std::setw(10) << "median"
               << std::setw(9) << "objects"
               << std::setw(9) << "strings";
        Log::Log(LogType::Help, header.str() + "\n");

        int failures = 0;
        for (fs::path const& file : files)
        {
            BenchResult result = MeasureBenchmark(file, _repeats);

            if (result.error.empty() == false)
            {
                failures++;
                Log::Log(LogType::Error, "  [FAIL] " + result.name + "\n    " + result.error + "\n");
                continue;
            }

            std::ostringstream row;
            row << std::left << std::setw(13) << result.name
                << std::right << std::setw(8) << result.staticInstructions
                << std::setw(15) << Grouped(result.executed)
                << std::fixed << std::setprecision(2)
                << std::setw(10) << result.minMs
                << std::setw(10) << result.medianMs
                << std::setw(9) << result.objectsPerRun
                << std::setw(9) << result.stringsPerRun;
            Log::Log(LogType::Info, row.str() + "\n");
            Log::Log(LogType::Help, "             " + TopOpcodes(result, 4) + "\n");
        }

        Log::Log(LogType::Help, "\ncode = instructions of the bytecode, executed = instructions run once, objects/strings = created by one run (no GC)\n");
        return failures;
    }
}

#endif
