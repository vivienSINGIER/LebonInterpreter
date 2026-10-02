#include "main.h"

#include <iostream>

#include <vector>

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h"

#include <windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    
    Lexer lexer(fs::path("../../res/Lebon/test.lbn"));
    lexer.Scan();
    lexer.DisplayTokens();

    Parser program = Parser(lexer.GetTokens());
    program.Parse();
    
    return ErrorManager::Code();
}
