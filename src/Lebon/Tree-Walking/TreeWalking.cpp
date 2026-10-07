#ifndef TREE_WALKING_CPP_INCLUDED
#define TREE_WALKING_CPP_INCLUDED

#include "TreeWalking.h"

#include <iostream>

using namespace RUNTIME;


void TreeWalking::Run(Program& _p)
{
    _p.Accept(*this);
}

void TreeWalking::Visit(NumberLiteral& _n)
{
    m_result = std::stof(_n.litteral);
}

void TreeWalking::Visit(StringLiteral& _s)
{
    m_result = _s.value;
}

void TreeWalking::Visit(BooleanLiteral& _b)
{
    m_result = _b.value; 
}

void TreeWalking::Visit(Identifier& _i)
{
    Value* v = m_env->Lookup(_i.name);
    if (!v)
        throw RuntimeError{ "unknown identifier '" + _i.name + "'", _i.row, _i.column };
    if (std::holds_alternative<std::monostate>(*v))
        throw RuntimeError{ "'" + _i.name + "' has no value yet", _i.row, _i.column };
    m_result = *v;
}

void TreeWalking::Visit(UnaryExpr& _u)
{
    Value n = Eval(*_u.operand);
    auto f = std::get_if<float>(&n);
    if (f)
    {
        m_result = -*f;
        return;
    }
    throw RuntimeError{ "operands must be numbers", _u.row, _u.column };
}

void TreeWalking::Visit(BinaryExpr& _b)
{
    Value left = Eval(*_b.left);
    Value right = Eval(*_b.right);
    auto lf = std::get_if<float>(&left);
    auto rf = std::get_if<float>(&right);
    auto rs = std::get_if<std::string>(&right);
    auto ls = std::get_if<std::string>(&left);

    switch (_b.op)
    {
    case TokenType::ADD:
        if (lf && rf) { m_result = *lf + *rf; return;}
        if (ls && rs) { m_result = *ls + *rs; return;}
        throw RuntimeError{"operands must be the same type", _b.row, _b.column };
        break;
    case TokenType::EQ:
        m_result = left == right; return;
    case TokenType::NEQ:
        m_result = left != right; return;
    default:
        break;
    }
    
    if ( !lf && !rf) { throw RuntimeError{ "operands must be numbers", _b.row, _b.column }; }
    
    switch (_b.op)
    {
    case TokenType::SUB: m_result = *lf - *rf; return;
    case TokenType::MUL: m_result = *lf * *rf; return;
    case TokenType::DIV:
        if ( *rf != 0) { m_result = *lf / *rf; return;}
        throw RuntimeError{ "division by zero", _b.row, _b.column }; 
        break;
    case TokenType::LT: m_result = *lf < *rf; return;
    case TokenType::GT: m_result = *lf > *rf; return;
    case TokenType::LE: m_result = *lf <= *rf; return;
    case TokenType::GE: m_result = *lf >= *rf; return;
    default:
        throw RuntimeError{ "unknown binary operator", _b.row, _b.column };
    }
}

void TreeWalking::Visit(AssignExpr& _a)
{
    Value v = Eval(*_a.value);
    Value* slot = m_env->Lookup(_a.name);
    if (!slot)
        throw RuntimeError{ "unknown identifier '" + _a.name + "'", _a.row, _a.column };
    *slot = v;
    m_result = std::move(v); 
}

void TreeWalking::Visit(CallExpr& _c)
{
    Value callee = Eval(*_c.callee);
    auto* fnPtr = std::get_if<std::shared_ptr<Callable>>(&callee);
    if (!fnPtr)
        throw RuntimeError{ "expression isn't callable", _c.row, _c.column };
    Callable& fn = **fnPtr;

    std::vector<Value> args;
    for (auto& a : _c.args)
        args.push_back(Eval(*a));

    if (fn.native)                       
    {
        m_result = fn.native(args);
        return;
    }

    if (args.size() != fn.decl->params.size())
        throw RuntimeError{ "wrong argument count", _c.row, _c.column };

    auto saved = m_env;
    m_env = std::make_shared<Environment>(fn.closure);     
    for (size_t i = 0; i < args.size(); i++)
        m_env->Define(fn.decl->params[i].name, args[i]);

    for (auto& s : fn.decl->body->statements)              
    {
        s->Accept(*this);
        if (m_returning) break;
    }

    m_env = saved;
    if (!m_returning)
        m_result = Value{};                               
    m_returning = false;                                  
}

void TreeWalking::Visit(VarDecl& _v)
{
    Value v;
    if ( _v.init ) v = Eval(*_v.init);
    m_env->Define(_v.name, v);
}

void TreeWalking::Visit(ExprStmt& _e)
{
    Eval(*_e.expr);
}

void TreeWalking::Visit(ReturnStmt& _r)
{
    m_result = _r.value ? Eval(*_r.value) : Value{};
    m_returning = true;
}

void TreeWalking::Visit(Block& _b)
{
    auto previous = m_env;
    m_env = std::make_shared<Environment>(previous);

    for (auto& s : _b.statements)
    {
        s->Accept(*this);
        if (m_returning) break;
    }

    m_env = previous;
}

void TreeWalking::Visit(FuncDecl& _f)
{
    auto fn = std::make_shared<Callable>();
    fn->name = _f.name;
    fn->decl = &_f;
    fn->closure = m_env;
    m_env->Define(_f.name, fn);
}

void TreeWalking::Visit(Program& _p)
{
    m_env = std::make_shared<Environment>();
    DefineBuiltIns();                 

    for (auto& s : _p.statements)
        s->Accept(*this);
}

void TreeWalking::Visit(IfStmt& _i)
{
    Value cond = Eval(*_i.condition);
    auto b = std::get_if<bool>(&cond);
    if (!b)
        throw RuntimeError{ "condition must be a boolean", _i.row, _i.column };
    if (*b)
        _i.thenBranch->Accept(*this);
    else if (_i.elseBranch)
        _i.elseBranch->Accept(*this);
}

void TreeWalking::DefineBuiltIns()
{
    auto afise = std::make_shared<Callable>();
    afise->name = "afise";
    afise->native = [](std::vector<Value>& _args) -> Value
    {
        std::cout << ToString(_args[0]) << "\n";
        return {};
    };
    m_env->Define("afise", afise);
}

#endif
