#ifndef PARSER_AST_PRINTER_H_INCLUDED
#define PARSER_AST_PRINTER_H_INCLUDED

#include <iostream>

#include "AST.h"

// Debug visitor: dumps the tree as an indented outline
class AstPrinter : public Visitor
{
public:
	explicit AstPrinter(std::ostream& _out = std::cout) : out(_out) {}

	void Print(Node& _root) { _root.Accept(*this); }

	void Visit(NumberLiteral& _node) override;
	void Visit(StringLiteral& _node) override;
	void Visit(BooleanLiteral& _node) override;
	void Visit(Identifier& _node) override;
	void Visit(UnaryExpr& _node) override;
	void Visit(BinaryExpr& _node) override;
	void Visit(AssignExpr& _node) override;
	void Visit(CallExpr& _node) override;
	void Visit(VarDecl& _node) override;
	void Visit(ExprStmt& _node) override;
	void Visit(ReturnStmt& _node) override;
	void Visit(Block& _node) override;
	void Visit(FuncDecl& _node) override;
	void Visit(Program& _node) override;

private:
	std::ostream& out;
	int depth = 0;

	std::ostream& Line();
	void Child(Node* _node);
};

#endif // !PARSER_AST_PRINTER_H_INCLUDED