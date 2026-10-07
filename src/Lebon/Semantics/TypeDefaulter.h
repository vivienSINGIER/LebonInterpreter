#ifndef SEMANTICS_TYPEDEFAULTER_H_DEFINED
#define SEMANTICS_TYPEDEFAULTER_H_DEFINED

#include "../Parser/AST.h"
#include "../core/Symbols.hpp"

namespace Semantics
{
    // Second, smaller walk run by the Analyser once every constraint of the program is known.
    // An addition accepts numbers or strings, so nothing may ever decide its type :
    //
    //   zafer fibo(n) ouver ran fibo(n mwin 1) èk fibo(n mwin 2) fèrm
    //
    // Such an addition is given the type number. Its type variable is shared with everything
    // it was unified with (here the return type of fibo), so they all become numbers at once.
    class TypeDefaulter : public Visitor
    {
    public:
        explicit TypeDefaulter(TypeTable& _types) : m_types(_types) {}

        // Returns how many additions were defaulted
        uint32_t Run(Program& _program);

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
        TypeTable& m_types;
        uint32_t m_defaulted = 0;
    };

}

#endif
