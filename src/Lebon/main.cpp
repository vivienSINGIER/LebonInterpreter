#include "main.h"


#include "core/Error.h"
#include "Test/Test.hpp"
#include <windows.h>

#include <windows.h>

// Sans argument   : les tests, puis le programme de démo avec les tokens, l'AST et le bytecode
// --tests         : les tests seulement
// <fichier>       : ce fichier avec les tokens, l'AST et le bytecode
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    std::string arg = argc > 1 ? argv[1] : "";

    if (arg.empty() == false && arg != "--tests")
        return Test::RunFile(fs::path(arg), false) == Error::ErrorCode::Ok ? 0 : 1;

    int failures = Test::RunAllTests();
    if (arg == "--tests")
        return failures == 0 ? 0 : 1;

    // Le second argument à true affiche aussi les tokens, l'AST et le bytecode
    fs::path demo = "../../res/Lebon/tests/vm/conditions.lbn";
    fs::path tests;
    if (Test::FindTestsDir(tests).IsOk())
        demo = tests / "vm" / "conditions.lbn";

    Log::Log(LogType::PromptInfo, "\n[demo] vm/conditions.lbn\n");
    Error::ErrorCode code = Test::RunFile(demo, false);

    return failures == 0 && code == Error::ErrorCode::Ok ? 0 : 1;
}
