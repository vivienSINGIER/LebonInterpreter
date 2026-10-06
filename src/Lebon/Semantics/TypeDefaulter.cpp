#include "TypeDefaulter.h"

namespace Semantics
{

    uint32_t TypeDefaulter::Run(Program& _program)
    {
        m_defaulted = 0;
        _program.Accept(*this);
        return m_defaulted;
    }

    void TypeDefaulter::Visit(Program& _p)
    {
        for (auto& s : _p.statements)
            s->Accept(*this);
    }

    void TypeDefaulter::Visit(VarDecl& _varDecl)
    {
        if (_varDecl.init != nullptr)
            _varDecl.init->Accept(*this);
    }

    void TypeDefaulter::Visit(Identifier&)
    {
    }

    void TypeDefaulter::Visit(AssignExpr& _expr)
    {
        _expr.value->Accept(*this);
    }

    void TypeDefaulter::Visit(Block& _b)
    {
        for (auto& s : _b.statements)
            s->Accept(*this);
    }

    void TypeDefaulter::Visit(NumberLiteral&)
    {
    }

    void TypeDefaulter::Visit(StringLiteral&)
    {
    }

    void TypeDefaulter::Visit(BooleanLiteral&)
    {
    }

    void TypeDefaulter::Visit(ExprStmt& _expr)
    {
        _expr.expr->Accept(*this);
    }

    void TypeDefaulter::Visit(FuncDecl& _func)
    {
        for (auto& st : _func.body->statements)
            st->Accept(*this);
    }

    void TypeDefaulter::Visit(CallExpr& _call)
    {
        for (auto& arg : _call.args)
            arg->Accept(*this);
    }

    void TypeDefaulter::Visit(ReturnStmt& _rtrn)
    {
        if (_rtrn.value != nullptr)
            _rtrn.value->Accept(*this);
    }

    void TypeDefaulter::Visit(UnaryExpr& _expr)
    {
        _expr.operand->Accept(*this);
    }

    void TypeDefaulter::Visit(BinaryExpr& _expr)
    {
        _expr.left->Accept(*this);
        _expr.right->Accept(*this);

        if (_expr.op != TokenType::ADD || _expr.typeVar == InvalidTypeVar)
            return;

        // Still unknown after the whole program was analysed : no constraint will ever pick between number and string
        if (m_types.Get(_expr.typeVar) == InferredType::Unknown)
        {
            m_types.Bind(_expr.typeVar, InferredType::Number);
            m_defaulted++;
        }
    }
}
