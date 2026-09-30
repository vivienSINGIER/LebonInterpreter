#include "Parser.h"

Parser::Parser(std::vector<Token> _tokens)
{

}

std::unique_ptr<Program> Parser::Parse()
{
    return std::unique_ptr<Program>();
}

Token const& Parser::Peek() const
{
    return m_tokens[m_current];
}

Token const& Parser::Previous() const
{
    // TODO: insérer une instruction return ici
}

bool Parser::IsAtEnd() const
{
    return false;
}

Token const& Parser::Advance()
{
    // TODO: insérer une instruction return ici
}

bool Parser::Check(TokenType) const
{
    return false;
}

bool Parser::Match(std::initializer_list<TokenType> _list)
{
    for (TokenType type : _list)
        if (type == Peek().type)
            return true;

    return false;
}

Token const& Parser::Consume(TokenType, std::string_view _msg)
{
    // TODO: insérer une instruction return ici
}

NodePtr Parser::Statement()
{
    return NodePtr();
}

NodePtr Parser::VarDeclaration()
{
    return NodePtr();
}

NodePtr Parser::FuncDeclaration()
{
    return NodePtr();
}

std::unique_ptr<Block> Parser::BlockStatement()
{
    return std::unique_ptr<Block>();
}

NodePtr Parser::Expression()
{
    return NodePtr();
}

NodePtr Parser::Term()
{
    auto left = Factor();
    while (Match({ TokenType::MUL, TokenType::DIV }))
    {
        auto op = Previous();
        auto right = Factor();
        left = std::make_unique<BinaryExpr>();
    }
    return left;
}

NodePtr Parser::Factor()
{
    return NodePtr();
}

