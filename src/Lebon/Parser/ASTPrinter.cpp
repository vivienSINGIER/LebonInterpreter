#include "ASTPrinter.h"

static char const* OpSymbol(TokenType _op)
{
	switch (_op)
	{
	case TokenType::ADD: return "+";
	case TokenType::SUB: return "-";
	case TokenType::MUL: return "*";
	case TokenType::DIV: return "/";
	default: return "?";
	}
}

std::ostream& AstPrinter::Line()
{
	for (int i = 0; i < depth; i++)
		out << "  ";
	return out;
}

void AstPrinter::Child(Node* _node)
{
	depth++;
	if (_node)
		_node->Accept(*this);
	else
		Line() << "<null>\n";
	depth--;
}

void AstPrinter::Visit(NumberLiteral& _node) { Line() << "Number " << _node.value << "\n"; }
void AstPrinter::Visit(StringLiteral& _node) { Line() << "String \"" << _node.value << "\"\n"; }
void AstPrinter::Visit(BooleanLiteral& _node) { Line() << "Boolean " << (_node.value ? "true" : "false") << "\n"; }
void AstPrinter::Visit(Identifier& _node) { Line() << "Identifier " << _node.name << "\n"; }

void AstPrinter::Visit(UnaryExpr& _node)
{
	Line() << "Unary " << OpSymbol(_node.op) << "\n";
	Child(_node.operand.get());
}

void AstPrinter::Visit(BinaryExpr& _node)
{
	Line() << "Binary " << OpSymbol(_node.op) << "\n";
	Child(_node.left.get());
	Child(_node.right.get());
}

void AstPrinter::Visit(AssignExpr& _node)
{
	Line() << "Assign " << _node.name << "\n";
	Child(_node.value.get());
}

void AstPrinter::Visit(CallExpr& _node)
{
	Line() << "Call\n";
	Child(_node.callee.get());
	for (NodePtr& arg : _node.args)
		Child(arg.get());
}

void AstPrinter::Visit(VarDecl& _node)
{
	Line() << "VarDecl " << _node.name << "\n";
	if (_node.init)
		Child(_node.init.get());
}

void AstPrinter::Visit(ExprStmt& _node)
{
	Line() << "ExprStmt\n";
	Child(_node.expr.get());
}

void AstPrinter::Visit(ReturnStmt& _node)
{
	Line() << "Return\n";
	if (_node.value)
		Child(_node.value.get());
}

void AstPrinter::Visit(Block& _node)
{
	Line() << "Block\n";
	for (NodePtr& stmt : _node.statements)
		Child(stmt.get());
}

void AstPrinter::Visit(FuncDecl& _node)
{
	Line() << "FuncDecl " << _node.name << "(";
	for (size_t i = 0; i < _node.params.size(); i++)
		out << (i ? ", " : "") << _node.params[i].name;
	out << ")\n";
	Child(_node.body.get());
}

void AstPrinter::Visit(Program& _node)
{
	Line() << "Program\n";
	for (NodePtr& stmt : _node.statements)
		Child(stmt.get());
}