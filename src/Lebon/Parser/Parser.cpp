#include "Parser.h"

Parser::Parser(std::vector<Token> _tokens) : m_tokens(std::move(_tokens))
{
	if (m_tokens.empty() || m_tokens.back().type != TokenType::END_OF_FILE)
	{
		int row = m_tokens.empty() ? 1 : m_tokens.back().row;
		int column = m_tokens.empty() ? 1 : m_tokens.back().column;

		m_tokens.push_back({ TokenType::END_OF_FILE, {}, row, column });
	}
}

std::unique_ptr<Program> Parser::Parse()
{
	auto program = MakeNode<Program>(Peek());

	while (!IsAtEnd())
	{
		NodePtr stmt = Statement();
		if (!stmt)
			return nullptr;

		program->statements.push_back(std::move(stmt));
	}
	return program;
}

Token const& Parser::Peek() const 
{ 
	return m_tokens[current]; 
}

Token const& Parser::Previous() const 
{ 
	return m_tokens[current - 1]; 
}

bool Parser::IsAtEnd() const 
{
	return Peek().type == TokenType::END_OF_FILE; 
}

bool Parser::Check(TokenType _type) const 
{ 
	return Peek().type == _type;
}

Token const& Parser::Advance()
{
	if (IsAtEnd() == false)
		current++;

	return Previous();
}

bool Parser::Match(std::initializer_list<TokenType> _types)
{
	for (TokenType type : _types)
	{
		if (Check(type))
		{
			Advance();
			return true;
		}
	}
	return false;
}

bool Parser::Consume(TokenType _type, std::string const& _message)
{
	if (Check(_type))
	{
		Advance();
		return true;
	}
	return Fail(Peek(), _message);
}

bool Parser::Fail(Token const& _at, std::string const& _message)
{
	if (error.IsOk()) // keep the first error only
	{
		error = Error::Syntax(_message + " (line " + std::to_string(_at.row)
			+ ", column " + std::to_string(_at.column) + ")");
	}
	return false;
}

NodePtr Parser::Statement()
{
	if (Match({ TokenType::VAR_DECLARATION })) return VarDeclaration();
	if (Match({ TokenType::FUNC_DECLARATION })) return FuncDeclaration();
	if (Match({ TokenType::RETURN })) return ReturnStatement();
	if (Check(TokenType::SCOPE_START)) return BlockStatement();
	return ExpressionStatement();
}

NodePtr Parser::VarDeclaration()
{
	auto decl = MakeNode<VarDecl>(Previous());

	if (Consume(TokenType::IDENTIFIER, "expected variable name") == false)
		return nullptr;

	decl->name = Previous().type;

	if (Match({ TokenType::ASSIGN }))
	{
		decl->init = Expression();
		if (!decl->init)
			return nullptr;
	}

	if (Consume(TokenType::SEMICOLON, "expected ';' after variable declaration") == false)
		return nullptr;

	return decl;
}

NodePtr Parser::FuncDeclaration()
{
	auto func = MakeNode<FuncDecl>(Previous());

	if (Consume(TokenType::IDENTIFIER, "expected function name") == false)
		return nullptr;
	func->name = Previous().type;

	if (!Consume(TokenType::L_PARENTHESIS, "expected '(' after function name"))
		return nullptr;

	if (!Check(TokenType::R_PARENTHESIS))
	{
		do
		{
			if (!Consume(TokenType::IDENTIFIER, "expected parameter name"))
				return nullptr;
			func->params.push_back(Previous().type);
		} while (Match({ TokenType::COMMA }));
	}

	if (!Consume(TokenType::R_PARENTHESIS, "expected ')' after parameters"))
		return nullptr;

	func->body = BlockStatement();
	if (!func->body)
		return nullptr;
	return func;
}

NodePtr Parser::ReturnStatement()
{
	auto ret = MakeNode<ReturnStmt>(Previous());

	if (!Check(TokenType::SEMICOLON))
	{
		ret->value = Expression();
		if (!ret->value)
			return nullptr;
	}

	if (!Consume(TokenType::SEMICOLON, "expected ';' after return"))
		return nullptr;