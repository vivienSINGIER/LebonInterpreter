#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <initializer_list>
#include <string>
#include <vector>

#include "../Lexer/Tokens.hpp"
#include "../core/Error.h"
#include "AST.h"

/*
 * Grammaire :
 *
 * COMMENT tokens are dropped before parsing.
 * Blank lines (NEWLINE*) are allowed between statements and before a function body.
 *
 * program        = { statement } END_OF_FILE ;
 * statement      = varDecl | funcDecl | returnStmt | block | exprStmt ;
 * varDecl        = VAR_DECLARATION IDENTIFIER [ ASSIGN expression ] end ;
 * funcDecl       = FUNC_DECLARATION IDENTIFIER "(" [ IDENTIFIER { "," IDENTIFIER } ] ")" block ;
 * returnStmt     = RETURN [ expression ] end ;
 * block          = SCOPE_START { statement } SCOPE_END ;
 * exprStmt       = expression end ;
 * end            = NEWLINE | (before) SCOPE_END | (before) END_OF_FILE ;
 *
 * expression     = assignment ;
 * assignment     = IDENTIFIER ASSIGN assignment | additive ;
 * additive       = multiplicative { ( ADD | SUB ) multiplicative } ;
 * multiplicative = unary { ( MUL | DIV ) unary } ;
 * unary          = SUB unary | call ;
 * call           = primary { "(" [ expression { "," expression } ] ")" } ;
 * primary        = NUMBER | STRING | TRUE | FALSE | IDENTIFIER | "(" expression ")" ;
 */

class Parser
{
public:
	explicit Parser(std::vector<Token> _tokens);

	std::unique_ptr<Program> Parse();

	void Synchronize();

private:
	std::vector<Token> m_tokens;
	size_t current = 0;
	Error m_error;
	bool m_panic = false;

	// Token primitives
	Token const& Peek() const;
	Token const& Previous() const;
	Token const& Advance();
	bool IsAtEnd() const;
	bool Check(TokenType _type) const;
	bool Check(std::initializer_list<TokenType> _types) const;
	bool Match(std::initializer_list<TokenType> _types);
	bool Consume(TokenType _type, std::string const& _message);
	bool Fail(Token const& _at, std::string const& _message);
	bool EndOfStatement();
	void SkipNewLines();

	// Statements
	NodePtr Statement();
	NodePtr VarDeclaration();
	NodePtr FuncDeclaration();
	NodePtr ReturnStatement();
	std::unique_ptr<Block> BlockStatement();
	NodePtr ExpressionStatement();

	// Expressions
	NodePtr Expression();
	NodePtr Assignment();
	NodePtr Additive();
	NodePtr Multiplicative();
	NodePtr Unary();
	NodePtr Call();
	NodePtr Primary();

	template <typename T>
	std::unique_ptr<T> MakeNode(Token const& _at)
	{
		auto node = std::make_unique<T>();
		node->row = _at.row;
		node->column = _at.column;
		return node;
	}
};

#endif // !PARSER_H_INCLUDED