#include "main.h"

#include <iostream>

#include <vector>

#include "core/FileHelper.h"
#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"
#include "Parser/Parser.h" 
#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"

int main()
{
    Lexer lexer(fs::path("../../res/Lebon/test.lbn"));
    lexer.Scan();
    lexer.DisplayTokens();

    Parser parser(lexer.GetTokens());
    std::unique_ptr<Program> program = parser.Parse();

    if (program)
    {
        AstPrinter printer;          // écrit sur std::cout par défaut
        printer.Print(*program);     // équivalent à program->Accept(printer);
    }

    return ErrorManager::Code();
}
