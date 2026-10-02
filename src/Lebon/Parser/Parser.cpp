#include "Parser.h"

Parser::Parser(std::vector<Token> _tokens)
{
	// Comments carry no meaning for the parser
	m_tokens.reserve(_tokens.size() + 1);
	for (Token& token : _tokens)
	{
		if (token.type != TokenType::COMMENT)
			m_tokens.push_back(std::move(token));
	}

	if (m_tokens.empty() || m_tokens.back().type != TokenType::END_OF_FILE)
	{
		uint32_t row = m_tokens.empty() ? 1 : m_tokens.back().row;
		uint32_t column = m_tokens.empty() ? 1 : m_tokens.back().column;

		Token eof = { TokenType::END_OF_FILE, {}, row, column };
		m_tokens.push_back(eof);
	}
}

std::unique_ptr<Program> Parser::Parse()
{
	auto program = MakeNode<Program>(Peek());

	while (true)
	{
		SkipNewLines();
		if (IsAtEnd())
			break;

		NodePtr stmt = Statement();
		if (!stmt)
		{
			Synchronize();
			m_panic = false;
			if (Check(TokenType::SCOPE_END))
				Advance();
			continue;           // on passe à l'instruction suivante
		}

		program->statements.push_back(std::move(stmt));
	}
	return program;
}

void Parser::Synchronize()
{
	while (IsAtEnd() == false)
	{
		if (Match({ TokenType::NEWLINE }))
			return;
		if (Check(TokenType::SCOPE_END))
			return;
		Advance();
	}
}

Token const& Parser::Peek() const { return m_tokens[current]; }
Token const& Parser::Previous() const { return m_tokens[current - 1]; }
bool Parser::IsAtEnd() const { return Peek().type == TokenType::END_OF_FILE; }
bool Parser::Check(TokenType _type) const { return Peek().type == _type; }

bool Parser::Check(std::initializer_list<TokenType> _types) const
{
	for (TokenType type : _types)
	{
		if (Check(type))
			return true;
	}
	return false;
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
	if (m_panic)
		return false;

	m_panic = true;
	m_error = Error::Syntax(_message, _at.row, _at.column);
	ErrorManager::LogError(m_error);

	return false;
}

bool Parser::EndOfStatement()
{
	if (Match({ TokenType::NEWLINE }))
		return true;

	if (Check(TokenType::SCOPE_END) || IsAtEnd())
		return true;

	return Fail(Peek(), "expected end of statement");
}

void Parser::SkipNewLines()
{
	while (Match({ TokenType::NEWLINE }));
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
	auto decla = MakeNode<VarDecl>(Previous());

	if (Consume(TokenType::IDENTIFIER, "expected variable name") == false)
		return nullptr;

	decla->name = Previous().literal;

	if (Match({ TokenType::ASSIGN }))
	{
		decla->init = Expression();
		if (!decla->init)
			return nullptr;
	}
		
	if (Check({ TokenType::IDENTIFIER, TokenType::NUMBER}))
	{
		Fail(Peek(), "expected assign operator");
		return nullptr;
	}

	if (EndOfStatement() == false)
		return nullptr;

	return decla;
}

NodePtr Parser::FuncDeclaration()
{
	auto func = MakeNode<FuncDecl>(Previous());

	if (!Consume(TokenType::IDENTIFIER, "expected function name"))
		return nullptr;
	func->name = Previous().literal;

	if (!Consume(TokenType::L_PARENTHESIS, "expected '(' after function name"))
		return nullptr;

	if (!Check(TokenType::R_PARENTHESIS))
	{
		do
		{
			if (Consume(TokenType::IDENTIFIER, "expected parameter name") == false)
				return nullptr;

			Token const& param = Previous();
			func->params.push_back({ param.literal, param.row, param.column });

		} while (Match({ TokenType::COMMA }));
	}

	if (Consume(TokenType::R_PARENTHESIS, "expected ')' after parameters") == false)
		return nullptr;

	SkipNewLines();
	func->body = BlockStatement();

	if (!func->body)
		return nullptr;

	return func;
}

NodePtr Parser::ReturnStatement()
{
	auto ret = MakeNode<ReturnStmt>(Previous());

	if (Check(TokenType::NEWLINE) == false && Check(TokenType::SCOPE_END) == false
		&& IsAtEnd() == false)
	{
		ret->value = Expression();

		if (!ret->value)
			return nullptr;
	}

	if (EndOfStatement() == false)
		return nullptr;

	return ret;
}

std::unique_ptr<Block> Parser::BlockStatement()
{
	if (Consume(TokenType::SCOPE_START, "expected start of block") == false)
		return nullptr;
	auto block = MakeNode<Block>(Previous());

	while (true)
	{
		SkipNewLines();
		if (Check(TokenType::SCOPE_END) || IsAtEnd())
			break;

		NodePtr stmt = Statement();
		if (!stmt)
		{
			Synchronize();
			m_panic = false;
			continue;        
		}

		block->statements.push_back(std::move(stmt));
	}

	if (Consume(TokenType::SCOPE_END, "expected end of block") == false)
		return nullptr;

	return block;
}

NodePtr Parser::ExpressionStatement()
{
	auto stmt = MakeNode<ExprStmt>(Peek());

	stmt->expr = Expression();
	if (!stmt->expr)
		return nullptr;

	if (EndOfStatement() == false)
		return nullptr;
	
	return stmt;
}

NodePtr Parser::Expression()
{
	return Assignment();
}

NodePtr Parser::Assignment()
{
	if (Check(TokenType::IDENTIFIER) && current + 1 < m_tokens.size() 
		&& m_tokens[current + 1].type == TokenType::ASSIGN)
	{
		Token const& name = Advance();
		Advance(); // '='

		auto assign = MakeNode<AssignExpr>(name);

		assign->name = name.literal;
		assign->value = Assignment();

		if (!assign->value)
			return nullptr;

		return assign;
	}
	return Additive();
}

NodePtr Parser::Additive()
{
	NodePtr left = Multiplicative();
	while (left && Match({ TokenType::ADD, TokenType::SUB }))
	{
		Token const& op = Previous();

		auto bin = MakeNode<BinaryExpr>(op);

		bin->op = op.type;
		bin->left = std::move(left);
		bin->right = Multiplicative();

		if (!bin->right)
			return nullptr;

		left = std::move(bin);
	}
	return left;
}

NodePtr Parser::Multiplicative()
{
	NodePtr left = Unary();
	while (left && Match({ TokenType::MUL, TokenType::DIV }))
	{
		Token const& op = Previous();

		auto bin = MakeNode<BinaryExpr>(op);

		bin->op = op.type;
		bin->left = std::move(left);
		bin->right = Unary();

		if (!bin->right)
			return nullptr;

		left = std::move(bin);
	}
	return left;
}

NodePtr Parser::Unary()
{
	if (Match({ TokenType::SUB }))
	{
		Token const& op = Previous();

		auto un = MakeNode<UnaryExpr>(op);

		un->op = op.type;
		un->operand = Unary();

		if (!un->operand)
			return nullptr;

		return un;
	}
	return Call();
}

NodePtr Parser::Call()
{
	NodePtr expr = Primary();

	while (expr && Match({ TokenType::L_PARENTHESIS }))
	{
		auto call = MakeNode<CallExpr>(Previous());
		call->callee = std::move(expr);

		if (Check(TokenType::R_PARENTHESIS) == false)
		{
			do
			{
				NodePtr arg = Expression();

				if (!arg)
					return nullptr;

				call->args.push_back(std::move(arg));

			} while (Match({ TokenType::COMMA }));
		}

		if (Consume(TokenType::R_PARENTHESIS, "expected ')' after arguments") == false)
			return nullptr;

		expr = std::move(call);
	}
	return expr;
}

NodePtr Parser::Primary()
{
	if (Match({ TokenType::NUMBER }))
	{
		auto num = MakeNode<NumberLiteral>(Previous());
		num->value = Previous().literal;
		return num;
	}
	if (Match({ TokenType::STRING }))
	{
		auto str = MakeNode<StringLiteral>(Previous());
		str->value = Previous().literal;
		return str;
	}
	if (Match({ TokenType::TRUE }))
	{
		auto b = MakeNode<BooleanLiteral>(Previous());
		b->value = true;
		return b;
	}
	if (Match({ TokenType::FALSE }))
	{
		auto b = MakeNode<BooleanLiteral>(Previous());
		b->value = false;
		return b;
	}
	if (Match({ TokenType::IDENTIFIER }))
	{
		auto id = MakeNode<Identifier>(Previous());
		id->name = Previous().literal;
		return id;
	}
	if (Match({ TokenType::L_PARENTHESIS }))
	{
		NodePtr inner = Expression();

		if (!inner || Consume(TokenType::R_PARENTHESIS, "expected ')' after expression") == false)
			return nullptr;

		return inner;
	}

	Fail(Peek(), "expected expression");
	return nullptr;
}
