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

    // Second argument set to true also prints the tokens and the AST
    Error::ErrorCode code = Test::RunFile("../../res/Lebon/tests/valid/program.lbn", true);

    return static_cast<int>(code);
}
