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
 * program     = { statement } END_OF_FILE ;
 * statement   = varDecl | funcDecl | returnStmt | block | exprStmt ;
 * varDecla     = VAR_DECLARATION IDENTIFIER [ ASSIGN expression ] NEW_LINE ;
 * funcDecla    = FUNC_DECLARATION IDENTIFIER "(" [ IDENTIFIER { "," IDENTIFIER } ] ")" block ;
 * returnStmt  = RETURN [ expression ] NEW_LINE ;
 * block       = SCOPE_START { statement } SCOPE_END ;
 * exprStmt    = expression NEW_LINE ;
 *
 * expression  = assignment ;
 * assignment  = IDENTIFIER ASSIGN assignment | additive ;
 * additive    = multiplicative { ( ADD | SUB ) multiplicative } ;
 * multi	   = unary { ( MUL | DIV ) unary } ;
 * unary       = SUB unary | call ;
 * call        = primary { "(" [ expression { "," expression } ] ")" } ;
 * primary     = NUMBER | TRUE | FALSE | IDENTIFIER | "(" expression ")" ;
 */

class Parser
{
public:
	explicit Parser(std::vector<Token> _tokens);

	std::unique_ptr<Program> Parse();

	Error const& GetError() const { return error; }

private:
	std::vector<Token> m_tokens;
	size_t current = 0;
	Error error;

	// Token primitives
	Token const& Peek() const;
	Token const& Previous() const;
	bool IsAtEnd() const;
	bool Check(TokenType _type) const;
	Token const& Advance();
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