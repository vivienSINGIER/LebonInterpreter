#ifndef INTERPRETER_H_INCLUDED
#define INTERPRETER_H_INCLUDED

#include <iostream>
#include "Value.hpp"

namespace Runtime
{
    class Interpreter : public Visitor
    {
    public:
        bool Run(Program& _program);

        void Visit(NumberLiteral&   _numberLiteral)     override;
        void Visit(StringLiteral&   _stringLiteral)     override;
        void Visit(BooleanLiteral&  _booleanLiteral)    override;
        void Visit(Identifier&      _identifier)        override;
        void Visit(VarDecl&         _varDecl)           override;
        void Visit(ExprStmt&        _exprStmt)          override;
        void Visit(AssignExpr&      _assignExpr)        override;
    
        void Visit(UnaryExpr&       _unaryExpr)         override;
        void Visit(BinaryExpr&      _binaryExpr)        override;
        void Visit(CallExpr&        _callExpr)          override;
        void Visit(ReturnStmt&      _returnStmt)        override;
        void Visit(Block&           _block)             override;
        void Visit(FuncDecl&        _funcDecl)          override;
        void Visit(Program&         _program)           override;
    
        std::string ToString(Value& _value);
        void DefineBuiltIns();

    private:
        std::shared_ptr<Environment> m_env;
        Value m_result;             
        bool  m_returning = false;  

        Value Eval(Node& _n) { _n.Accept(*this); return std::move(m_result); }
    };
}

#endif
