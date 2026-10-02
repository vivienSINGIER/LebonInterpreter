#ifndef PARSER_AST_H_INCLUDED
#define PARSER_AST_H_INCLUDED

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Lexer/Tokens.hpp"

struct Node
{
	uint32_t row = 0, column = 0;
	virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

// Expressions 
struct NumberLiteral : Node { std::string value; };
struct StringLiteral : Node { std::string value; };
struct BooleanLiteral : Node { bool value = false; };
struct Identifier : Node { std::string name; };
// END_OF_FILE par defaut 
struct UnaryExpr : Node { TokenType op = TokenType::END_OF_FILE; NodePtr operand; };
struct BinaryExpr : Node { TokenType op = TokenType::END_OF_FILE; NodePtr left, right; };
struct AssignExpr : Node { std::string name; NodePtr value; };
struct CallExpr : Node { NodePtr callee; std::vector<NodePtr> args; };

// Statements 
struct VarDecl : Node { std::string name; NodePtr init; }; 
struct ExprStmt : Node { NodePtr expr; };
struct ReturnStmt : Node { NodePtr value; };                    
struct Block : Node { std::vector<NodePtr> statements; };
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
};
struct Program : Node { std::vector<NodePtr> statements; };

#endif // !PARSER_AST_H_INCLUDED