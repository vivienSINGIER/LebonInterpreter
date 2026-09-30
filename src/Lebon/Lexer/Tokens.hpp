#ifndef TOKENS_HPP_DEFINED
#define TOKENS_HPP_DEFINED

#include <string>
#include <string_view>
#include <vector>

enum TokenType : int
{
    QUOTE,
    DOUBLE_QUOTE,
    L_PARENTHESIS,
    R_PARENTHESIS,
    
    VAR_DECLARATION,
    FUNC_DECLARATION,
    SCOPE_START,
    SCOPE_END,
    RETURN,
    COMMENT,
 
    BOOLEAN,
    ADD,
    SUB,
    MUL,
    DIV,
    ASSIGN,
    
    IDENTIFIER,
    NUMBER,
};

struct Token
{
    TokenType type;
    std::string_view value;
    int row, column;
};

static std::pair<std::string, TokenType> const tokenSpecs[] = {
    { "'", TokenType::QUOTE },
    { "\"", TokenType::DOUBLE_QUOTE },
    { "(", TokenType::L_PARENTHESIS },
    { ")", TokenType::R_PARENTHESIS },
};

#endif
