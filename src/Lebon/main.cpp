#include "main.h"


#include "core/Error.h"
#include "JIT/JitTests.h"
#include "Test/Test.hpp"

#include <windows.h>

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    Jit::RunSelfTests();

    // Second argument set to true also prints the tokens and the AST
    Error::ErrorCode code = Test::RunFile("../../res/Lebon/tests/valid/program.lbn", false);

    return static_cast<int>(code);
}
