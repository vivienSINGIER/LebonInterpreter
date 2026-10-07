#ifndef SEMANTICS_ANALYSER_H_DEFINED
#define SEMANTICS_ANALYSER_H_DEFINED

#include <string>

#include "../core/Error.h"
#include "../Parser/AST.h"
#include "../core/Symbols.hpp"

namespace Semantics
{
    
    class Analyser : public Visitor
    {
    public:
        Analyser() = default;
        ~Analyser() = default;
        
        bool Run(Program& _program);
        
        void Visit(Program& _p) override;
        void Visit(VarDecl& _varDecl) override;
        void Visit(Identifier& _identifier) override;
        void Visit(AssignExpr& _expr) override;
        void Visit(Block& _b) override;
        void Visit(NumberLiteral& _l) override;
        void Visit(StringLiteral& _l) override;
        void Visit(BooleanLiteral& _l) override;
        void Visit(ExprStmt& _expr) override;
        void Visit(FuncDecl& _func) override;
        void Visit(CallExpr& _call) override;
        void Visit(ReturnStmt& _rtrn) override;
        void Visit(IfStmt& _stmt) override;
        void Visit(UnaryExpr& _expr) override;
        void Visit(BinaryExpr& _expr) override;
        
    private:
        ScopeStack* m_stack = nullptr;
        TypeTable m_types;
        std::vector<Expr*> m_exprs;
        uint32_t m_errorCount = 0;
        
        void DefineBuiltIns();

        // Logs a semantics error and keeps going, the walk never stops on an error
        void Report(Node const& _at, std::string const& _message);
        void Report(uint32_t _row, uint32_t _column, std::string const& _message);
        
        InferredType ValueType(Expr const& _expr);
        TypeVar ValueVar(Expr const& _expr);
        
        void SetVar(Expr& _expr, Semantics::TypeVar _var);
    };
    
}

#endif