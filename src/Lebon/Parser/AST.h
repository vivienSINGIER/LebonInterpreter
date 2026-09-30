#ifndef PARSER_AST_H_INCLUDED
#define PARSER_AST_H_INCLUDED

#include <memory>
#include <string_view>
#include <vector>

#include "../Lexer/Tokens.hpp"

struct Node
{
	uint32_t row = 0, column = 0;
	virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

// ---- Expressions ----
struct NumberLiteral : Node { std::string value; };
struct BooleanLiteral : Node { bool value = false; };
struct Identifier : Node { std::string name; };
struct UnaryExpr : Node { TokenType op; NodePtr operand; };
struct BinaryExpr : Node { TokenType op; NodePtr left, right; };
struct AssignExpr : Node { std::string name; NodePtr value; };
struct CallExpr : Node { NodePtr callee; std::vector<NodePtr> args; };

// ---- Statements ----
struct VarDecl : Node { std::string name; NodePtr init; }; 
struct ExprStmt : Node { NodePtr expr; };
struct ReturnStmt : Node { NodePtr value; };                    
struct Block : Node { std::vector<NodePtr> statements; };
struct FuncDecl : Node
{
	std::string name;
	std::vector<std::string> params;
	std::unique_ptr<Block> body;
};
struct Program : Node { std::vector<NodePtr> statements; };

#endif // !PARSER_AST_H_INCLUDED