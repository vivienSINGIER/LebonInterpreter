#ifndef LEXER_H_DEFINED
#define LEXER_H_DEFINED

#include <vector>
#include "Tokens.hpp"

class Lexer
{
public:
    std::vector<Token>& GetTokens();

private:
    std::vector<Token> vToken;
};

#endif
