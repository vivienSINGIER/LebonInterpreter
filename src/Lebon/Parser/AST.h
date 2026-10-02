#ifndef PARSER_AST_H_INCLUDED
#define PARSER_AST_H_INCLUDED

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Lexer/Tokens.hpp"

struct NumberLiteral;
struct StringLiteral;
struct BooleanLiteral;
struct Identifier;
struct UnaryExpr;
struct BinaryExpr;
struct AssignExpr;
struct CallExpr;
struct VarDecl;
struct ExprStmt;
struct ReturnStmt;
struct Block;
struct Param;
struct FuncDecl;
struct Program;

struct Visitor
{
	virtual ~Visitor() = default;
	virtual void Visit(NumberLiteral&) = 0;
	virtual void Visit(StringLiteral&) = 0;
	virtual void Visit(BooleanLiteral&) = 0;
	virtual void Visit(Identifier&) = 0;
	virtual void Visit(UnaryExpr&) = 0;
	virtual void Visit(BinaryExpr&) = 0;
	virtual void Visit(AssignExpr&) = 0;
	virtual void Visit(CallExpr&) = 0;
	virtual void Visit(VarDecl&) = 0;
	virtual void Visit(ExprStmt&) = 0;
	virtual void Visit(ReturnStmt&) = 0;
	virtual void Visit(Block&) = 0;
	virtual void Visit(FuncDecl&) = 0;
	virtual void Visit(Program&) = 0;
};

struct Node
{
	uint32_t row = 0, column = 0;
	virtual void Accept(Visitor&) = 0;
	virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

// Expressions 
struct NumberLiteral : Node 
{ 
	std::string value;
	void Accept(Visitor& _visitor) override;
};

struct StringLiteral : Node 
{ 
	std::string value;
	void Accept(Visitor& _visitor) override;
};

struct BooleanLiteral : Node 
{ 
	bool value = false; 
	void Accept(Visitor& _visitor) override;
};

struct Identifier : Node 
{ 
	std::string name;
	void Accept(Visitor& _visitor) override;
};

// END_OF_FILE par defaut (NE DOIT PAS RESTER)
struct UnaryExpr : Node 
{
	TokenType op = TokenType::END_OF_FILE;
	NodePtr operand;
	void Accept(Visitor& _visitor) override;
};

struct BinaryExpr : Node 
{ 
	TokenType op = TokenType::END_OF_FILE; 
	NodePtr left, right; 
	void Accept(Visitor& _visitor) override;
};

struct AssignExpr : Node 
{ 
	std::string name;
	NodePtr value; 
	void Accept(Visitor& _visitor) override;
};

struct CallExpr : Node 
{ 
	NodePtr callee; 
	std::vector<NodePtr> args; 
	void Accept(Visitor& _visitor) override;
};

// Statements 
struct VarDecl : Node 
{ 
	std::string name; 
	NodePtr init;
	void Accept(Visitor& _visitor) override;
}; 

struct ExprStmt : Node 
{ 
	NodePtr expr;
	void Accept(Visitor& _visitor) override;
};

struct ReturnStmt : Node 
{
	NodePtr value;
	void Accept(Visitor& _visitor) override;
};                  

struct Block : Node 
{ 
	std::vector<NodePtr> statements; 
	void Accept(Visitor& _visitor) override;
};

struct Param
{
	std::string name;
	uint32_t row = 0, column = 0;
};

struct FuncDecl : Node
{
	std::string name;
	std::vector<Param> params;
	std::unique_ptr<Block> body;
	void Accept(Visitor& _visitor) override;
};

struct Program : Node 
{ 
	std::vector<NodePtr> statements; 
	void Accept(Visitor& _visitor) override;
};

#endif // !PARSER_AST_H_INCLUDED