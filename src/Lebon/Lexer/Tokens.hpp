#ifndef TOKENS_HPP_DEFINED
#define TOKENS_HPP_DEFINED

#include <string>
#include <string_view>
#include <vector>

enum TokenType : int
{
    BOOLEAN,
    OPERAND,
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
    
    IDENTIFIER,
    NUMBER,
};

enum OperandType : int 
{
    ADD, SUB, MUL, DIV, ASSIGN, EQUALS  
};

struct Token
{
    TokenType type;
    std::string_view value;
    int row, column;
};

struct TokenSpec
{
    std::string value;
    TokenType type;
};

static TokenSpec const tokenSpecs[] = {
    
};

#endif
