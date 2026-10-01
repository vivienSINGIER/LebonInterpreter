#include "main.h"

#include <iostream>

#include <vector>

#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"

int main()
{
    
    Lexer lexer("(((().,,.?? ifé 894 \"bonjour\" 42.99 \"toz");
    lexer.Scan();
    
    std::vector<Token> const& tokens = lexer.GetTokens();
    
    for (auto const& token : tokens)
        std::cout << "[ " << token.literal << " ]" << " ";
    
    return 0;
}
