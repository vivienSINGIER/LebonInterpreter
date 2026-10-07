#include "ASTPrinter.h"

#include <string>

static char const* OpSymbol(TokenType _op)
{
	switch (_op)
	{
	case TokenType::ADD: return "+";
	case TokenType::SUB: return "-";
	case TokenType::MUL: return "*";
	case TokenType::DIV: return "/";
	case TokenType::EQ:  return "==";
	case TokenType::NEQ: return "!=";
	case TokenType::LT:  return "<";
	case TokenType::GT:  return ">";
	case TokenType::LE:  return "<=";
	case TokenType::GE:  return ">=";
	default: return "?";
	}
}

static char const* TypeName(Semantics::InferredType _type)
{
	switch (_type)
	{
	case Semantics::InferredType::Unknown: return "unknown";
	case Semantics::InferredType::Bool:    return "bool";
	case Semantics::InferredType::String:  return "string";
	case Semantics::InferredType::Number:  return "number";
	case Semantics::InferredType::Void:    return "void";
	}
	return "unknown";
}

// " : number", the type the analyser gave to the expression
static std::string TypeTag(Expr const& _expr)
{
	return std::string(" : ") + TypeName(_expr.type);
}

// " #3", the id of the symbol the node is bound to, " #?" when it has none
static std::string SymbolTag(Semantics::SymbolId _id)
{
	if (_id == Semantics::InvalidSymbolId)
		return " #?";
	return " #" + std::to_string(_id);
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

void AstPrinter::Visit(NumberLiteral& _node) { Line() << "Number " << _node.litteral << TypeTag(_node) << "\n"; }
void AstPrinter::Visit(StringLiteral& _node) { Line() << "String \"" << _node.value << "\"" << TypeTag(_node) << "\n"; }
void AstPrinter::Visit(BooleanLiteral& _node) { Line() << "Boolean " << (_node.value ? "true" : "false") << TypeTag(_node) << "\n"; }
void AstPrinter::Visit(Identifier& _node) { Line() << "Identifier " << _node.name << SymbolTag(_node.symbol) << TypeTag(_node) << "\n"; }

void AstPrinter::Visit(UnaryExpr& _node)
{
	Line() << "Unary " << OpSymbol(_node.op) << TypeTag(_node) << "\n";
	Child(_node.operand.get());
}

void AstPrinter::Visit(BinaryExpr& _node)
{
	Line() << "Binary " << OpSymbol(_node.op) << TypeTag(_node) << "\n";
	Child(_node.left.get());
	Child(_node.right.get());
}

void AstPrinter::Visit(AssignExpr& _node)
{
	Line() << "Assign " << _node.name << SymbolTag(_node.symbol) << TypeTag(_node) << "\n";
	Child(_node.value.get());
}

void AstPrinter::Visit(CallExpr& _node)
{
	Line() << "Call" << SymbolTag(_node.symbol) << TypeTag(_node) << "\n";
	Child(_node.callee.get());
	for (ExprPtr& arg : _node.args)
		Child(arg.get());
}

void AstPrinter::Visit(VarDecl& _node)
{
	Line() << "VarDecl " << _node.name << SymbolTag(_node.symbol) << "\n";
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

void AstPrinter::Visit(IfStmt& _node)
{
	Line() << "If\n";
	Child(_node.condition.get());
	Child(_node.thenBranch.get());
	if (_node.elseBranch)
	{
		Line() << "Else\n";
		Child(_node.elseBranch.get());
	}
}

void AstPrinter::Visit(Block& _node)
{
	Line() << "Block\n";
	for (NodePtr& stmt : _node.statements)
		Child(stmt.get());
}

void AstPrinter::Visit(FuncDecl& _node)
{
	Line() << "FuncDecl " << _node.name << SymbolTag(_node.symbol) << " (";
	for (size_t i = 0; i < _node.params.size(); i++)
		out << (i ? ", " : "") << _node.params[i].name << SymbolTag(_node.params[i].symbol);
	out << ")\n";
	Child(_node.body.get());
}

void AstPrinter::Visit(Program& _node)
{
	Line() << "Program\n";
	for (NodePtr& stmt : _node.statements)
		Child(stmt.get());
}
