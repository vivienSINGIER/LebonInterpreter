#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <memory>
#include "../Lexer/Tokens.hpp"
#include "AST.h"

class Parser
{
public:
    explicit Parser(std::vector<Token> _tokens);
    std::unique_ptr<Program> Parse();

private:
    std::vector<Token> vTokens;
    size_t current = 0;

    // Primitives
    Token const& Peek() const;                 
    Token const& Previous() const;
    bool IsAtEnd() const;                     
    Token const& Advance();                    
    bool Check(TokenType) const;
    bool Match(std::initializer_list<TokenType>); 
    Token const& Consume(TokenType, std::string_view msg); 

    // Une méthode par règle
    NodePtr Statement();
    NodePtr VarDeclaration();
    NodePtr FuncDeclaration();
    std::unique_ptr<Block> BlockStatement();
    NodePtr Expression();
    NodePtr Term();
    NodePtr Factor();
};

#endif // !PARSER_H_INCLUDED