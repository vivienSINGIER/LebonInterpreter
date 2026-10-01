#include "main.h"

#include <iostream>

#include <vector>

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"

int main()
{
    Lexer lexer(fs::path("../../res/Lebon/test.lbn"));
    lexer.Scan();
    lexer.DisplayTokens();
    
    return ErrorManager::Code();
}
