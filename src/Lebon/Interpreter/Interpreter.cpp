#ifndef INTERPRETER_CPP_INCLUDED
#define INTERPRETER_CPP_INCLUDED

#include "Interpreter.h"
#include "core/Error.h"

namespace Runtime
{
    bool Interpreter::Run(Program& _program)
    {
        try { _program.Accept(*this); return true; }
        catch (RuntimeError& e)
        {
            ErrorManager::LogError(Error::Execution(e.message, e.row, e.column));
            return false;
        }
    }

    void Interpreter::Visit(NumberLiteral& _numberLiteral)
    {
        m_result = std::stof(_numberLiteral.value);
    }

    void Interpreter::Visit(StringLiteral& _stringLiteral)
    {
        m_result = _stringLiteral.value;
    }

    void Interpreter::Visit(BooleanLiteral& _booleanLiteral)
    {
        m_result = _booleanLiteral.value;
    }

    void Interpreter::Visit(Identifier& _identifier)
    {
        Value* v = m_env->Lookup(_identifier.name);
        if (!v)
            throw RuntimeError{ "unknown identifier '" + _identifier.name + "'", _identifier.row, _identifier.column };
        if (std::holds_alternative<std::monostate>(*v))
            throw RuntimeError{ "'" + _identifier.name + "' has no value yet", _identifier.row, _identifier.column };
        m_result = *v;
    }

    void Interpreter::Visit(AssignExpr& _assignExpr)
    {
        Value v = Eval(*_assignExpr.value);
        Value* slot = m_env->Lookup(_assignExpr.name);
        if (!slot)
            throw RuntimeError{ "unknown identifier '" + _assignExpr.name + "'", _assignExpr.row, _assignExpr.column };
        *slot = v;
        m_result = std::move(v);
    }

    void Interpreter::Visit(VarDecl& _varDecl)
    {
        Value v;                                  
        if (_varDecl.init) v = Eval(*_varDecl.init);
        m_env->Define(_varDecl.name, std::move(v));
    }

    void Interpreter::Visit(ExprStmt& _exprStmt)
    {
        Eval(*_exprStmt.expr);
    }

    void Interpreter::Visit(UnaryExpr& _unaryExpr)
    {
        Value v = Eval(*_unaryExpr.operand);
        auto* n = std::get_if<float>(&v);
        if (!n)
            throw RuntimeError{ "operand must be a number", _unaryExpr.row, _unaryExpr.column };
        m_result = -*n;
    }

    void Interpreter::Visit(BinaryExpr& _binaryExpr)
    {
        Value l = Eval(*_binaryExpr.left);
        Value r = Eval(*_binaryExpr.right);

        auto* ln = std::get_if<float>(&l);       auto* rn = std::get_if<float>(&r);
        auto* ls = std::get_if<std::string>(&l);  auto* rs = std::get_if<std::string>(&r);

        if (_binaryExpr.op == TokenType::ADD) 
        {
            if (ln && rn) { m_result = *ln + *rn; return; }
            if (ls && rs) { m_result = *ls + *rs; return; }
            throw RuntimeError{ "can't add these two values", _binaryExpr.row, _binaryExpr.column };
        }

        if (!ln || !rn)
            throw RuntimeError{ "operands must be numbers", _binaryExpr.row, _binaryExpr.column };

        switch (_binaryExpr.op)
        {
        case TokenType::SUB: m_result = *ln - *rn; break;
        case TokenType::MUL: m_result = *ln * *rn; break;
        case TokenType::DIV:
            if (*rn == 0.0)
                throw RuntimeError{ "division by zero", _binaryExpr.row, _binaryExpr.column };
            m_result = *ln / *rn;
            break;
        default: break;
        }
    }

    void Interpreter::Visit(CallExpr& _callExpr)
    {
        Value callee = Eval(*_callExpr.callee);
        auto* fnPtr = std::get_if<std::shared_ptr<Callable>>(&callee);
        if (!fnPtr)
            throw RuntimeError{ "expression isn't callable", _callExpr.row, _callExpr.column };
        Callable& fn = **fnPtr;

        std::vector<Value> args;
        for (auto& a : _callExpr.args)
            args.push_back(Eval(*a));

        if (fn.native)                      
        {
            m_result = fn.native(args);
            return;
        }

        if (args.size() != fn.decl->params.size())
            throw RuntimeError{ "wrong argument count", _callExpr.row, _callExpr.column };

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



    void Interpreter::Visit(ReturnStmt& _returnStmt)
    {
        m_result = _returnStmt.value ? Eval(*_returnStmt.value) : Value{};
        m_returning = true;
    }

    void Interpreter::Visit(Block& _block)
    {
        auto previous = m_env;
        m_env = std::make_shared<Environment>(previous);

        for (auto& s : _block.statements)
        {
            s->Accept(*this);
            if (m_returning) break;
        }

        m_env = previous;
    }

    void Interpreter::Visit(FuncDecl& _funcDecl)
    {
        auto fn = std::make_shared<Callable>();
        fn->name    = _funcDecl.name;
        fn->decl    = &_funcDecl;
        fn->closure = m_env;
        m_env->Define(_funcDecl.name, fn); 
    }

    void Interpreter::Visit(Program& _program)
    {
        m_env = std::make_shared<Environment>();
        DefineBuiltIns();                        

        for (auto& s : _program.statements)
            s->Accept(*this);
    }

    std::string Interpreter::ToString(Value& _value)
    {
        auto* f     = std::get_if<float>(&_value);
        auto* v    = std::get_if<std::string>(&_value);
        auto* b     = std::get_if<bool>(&_value);
        
        if (f)
            return std::to_string(*f);
        if (v)
            return *v;
        if (b)
        {
            if (*b)
                return "true";
            return "false";
        }
        return "-1";
    }

    void Interpreter::DefineBuiltIns()
    {
        auto afise = std::make_shared<Callable>();
        afise->name = "afise";
        afise->native = [&](std::vector<Value>& _args) -> Value
        {
            std::cout << ToString(_args[0]) << "\n";
            return {};
        };
        m_env->Define("afise", afise);
    }
}

#endif
