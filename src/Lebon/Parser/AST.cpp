#include "AST.h"

void NumberLiteral::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void StringLiteral::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void BooleanLiteral::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void Identifier::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void UnaryExpr::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void BinaryExpr::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void AssignExpr::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void CallExpr::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void VarDecl::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void ExprStmt::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void ReturnStmt::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void Block::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void FuncDecl::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}

void Program::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);
}
