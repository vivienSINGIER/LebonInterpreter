#include "main.h"


#include "core/Error.h"
#include "Test/Test.hpp"

#include <windows.h>

// Sans argument   : les tests, puis le programme de démo avec les tokens, l'AST et le bytecode
// --tests         : les tests seulement
// <fichier>       : ce fichier avec les tokens, l'AST et le bytecode
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    std::string arg = argc > 1 ? argv[1] : "";

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
