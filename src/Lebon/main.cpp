#include "main.h"

#include <windows.h>

#include "core/Error.h"
#include "Test/Test.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    // Second argument set to true also prints the tokens and the AST
    Error::ErrorCode code = Test::RunFile("../../res/Lebon/tests/valid/program.lbn", true);

    return code == Error::ErrorCode::Ok ? 0 : 1;
}
