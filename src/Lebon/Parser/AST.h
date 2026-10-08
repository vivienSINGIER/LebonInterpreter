#ifndef PARSER_AST_H_INCLUDED
#define PARSER_AST_H_INCLUDED

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Lexer/Tokens.hpp"
#include "../core/Symbols.hpp"

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
struct IfStmt;
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
	virtual void Visit(IfStmt&) = 0;
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

struct Expr : Node
{
	Semantics::TypeVar typeVar = Semantics::InvalidTypeVar;
	Semantics::InferredType type = Semantics::InferredType::Unknown;
};

using NodePtr = std::unique_ptr<Node>;
using ExprPtr = std::unique_ptr<Expr>;

// Expressions 
struct NumberLiteral : Expr 
{ 
	float value;
	std::string litteral;
	void Accept(Visitor& _visitor) override;
};

struct StringLiteral : Expr 
{ 
	std::string value;
	void Accept(Visitor& _visitor) override;
};

struct BooleanLiteral : Expr 
{ 
	bool value = false; 
	void Accept(Visitor& _visitor) override;
};

struct Identifier : Expr 
{ 
	std::string name;
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	void Accept(Visitor& _visitor) override;
};

// END_OF_FILE par defaut (NE DOIT PAS RESTER)
struct UnaryExpr : Expr 
{
	TokenType op = TokenType::END_OF_FILE;
	ExprPtr operand;
	void Accept(Visitor& _visitor) override;
};

struct BinaryExpr : Expr 
{ 
	TokenType op = TokenType::END_OF_FILE; 
	ExprPtr left, right; 
	void Accept(Visitor& _visitor) override;
};

struct AssignExpr : Expr 
{ 
	std::string name;
	ExprPtr value; 
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	void Accept(Visitor& _visitor) override;
};

struct CallExpr : Expr 
{ 
	ExprPtr callee; 
	std::vector<ExprPtr> args; 
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	void Accept(Visitor& _visitor) override;
};

// Statements 
struct VarDecl : Node 
{ 
	std::string name; 
	ExprPtr init;
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	void Accept(Visitor& _visitor) override;
}; 

struct ExprStmt : Node 
{ 
	ExprPtr expr;
	void Accept(Visitor& _visitor) override;
};

struct ReturnStmt : Node 
{
	ExprPtr value;
	void Accept(Visitor& _visitor) override;
};                  

struct Block : Node 
{ 
	std::vector<NodePtr> statements; 
	void Accept(Visitor& _visitor) override;
};

// kan condition ouver ... fèrm [ sinon-si condition ouver ... fèrm ] [ otreman ouver ... fèrm ]
struct IfStmt : Node
{
	ExprPtr condition;
	std::unique_ptr<Block> thenBranch;
	NodePtr elseBranch;		// un Block (otreman) ou un autre IfStmt (sinon-si), nullptr sans sinon
	void Accept(Visitor& _visitor) override;
};

struct Param
{
	std::string name;
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	uint32_t row = 0, column = 0;
};

struct FuncDecl : Node
{
	std::string name;
	std::vector<Param> params;
	std::unique_ptr<Block> body;
	Semantics::SymbolId symbol = Semantics::InvalidSymbolId;
	void Accept(Visitor& _visitor) override;
};

struct Program : Node 
{ 
	std::vector<NodePtr> statements; 
	Semantics::ScopeStack stack;
	void Accept(Visitor& _visitor) override;
};

#endif // !PARSER_AST_H_INCLUDED