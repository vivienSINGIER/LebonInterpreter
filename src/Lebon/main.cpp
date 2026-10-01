#include "main.h"

#include <iostream>

#include <vector>

#include "Lexer/Lexer.h"
#include "Lexer/Tokens.hpp"

int main()
{
    
    Lexer lexer("(((().,,.?? ifé 894 \"bonjour\" 42.99 \"toz\" koz ceci est un commentaire finkoz test koz reteest");
    lexer.Scan();
    lexer.DisplayTokens();
    
    return 0;
}
