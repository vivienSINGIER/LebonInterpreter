#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include <vector>
#include <string>
#include <memory>
#include "../Lexer/Tokens.hpp"

struct Node { virtual ~Node() = default; int row, column; };
using NodePtr = std::unique_ptr<Node>;

struct NumberLiteral : Node { double value; };
struct Identifier : Node { std::string_view name; };
struct BinaryExpr : Node { TokenType op; NodePtr left, right; };
struct VarDecl : Node { std::string_view name; NodePtr init; };
struct Block : Node { std::vector<NodePtr> statements; };
struct FuncDecl : Node { std::string_view name; std::vector<std::string_view> params; std::unique_ptr<Block> body; };
struct ReturnStmt : Node { NodePtr value; };
struct Program : Node { std::vector<NodePtr> statements; };

#endif