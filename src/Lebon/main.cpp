#include "main.h"


#include "core/Error.h"
#include "Test/Test.hpp"
#include "Test/Benchmark.hpp"

#include <windows.h>

// Sans argument   : les tests, puis le programme de démo avec les tokens, l'AST et le bytecode
// --tests         : les tests seulement
// --bench [N]     : les benchmarks de res/Lebon/benchmarks, N exécutions chronométrées chacun (5 par défaut)
// <fichier>       : ce fichier avec les tokens, l'AST et le bytecode
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    std::string arg = argc > 1 ? argv[1] : "";

    if (arg == "--bench")
    {
        int repeats = argc > 2 ? std::atoi(argv[2]) : 5;
        return Test::RunBenchmarks(repeats > 0 ? repeats : 5) == 0 ? 0 : 1;
    }

    if (arg.empty() == false && arg != "--tests")
        return Test::RunFile(fs::path(arg), true) == Error::ErrorCode::Ok ? 0 : 1;

    int failures = Test::RunAllTests();
    if (arg == "--tests")
        return failures == 0 ? 0 : 1;

    // Le second argument à true affiche aussi les tokens, l'AST et le bytecode
    fs::path demo = "../../res/Lebon/tests/valid/program.lbn";
    fs::path tests;
    if (Test::FindTestsDir(tests).IsOk())
        demo = tests / "valid" / "program.lbn";

    Log::Log(LogType::PromptInfo, "\n[demo] valid/program.lbn\n");
    Error::ErrorCode code = Test::RunFile(demo, true);

    return failures == 0 && code == Error::ErrorCode::Ok ? 0 : 1;
}
