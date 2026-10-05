#ifndef COMPILER_COMPILER_H_DEFINED
#define COMPILER_COMPILER_H_DEFINED

#include <memory>
#include <string>

#include "../Bytecode/Heap.hpp"
#include "../Bytecode/Prototype.hpp"
#include "../Parser/AST.h"

namespace Bytecode
{
    struct LocalVar
    {
        std::string name;
        uint8_t registre;
    };

    struct Scope
    {
        size_t localCount; 
        size_t firstTemp;
    };

    class Compiler : public Visitor
    {
    public:
        explicit Compiler(Heap& _heap) : m_heap(_heap) {}

        // Renvoie la fonction main ou nullptr si erreur 
        std::unique_ptr<Prototype> Compile(Program& _program);

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
        Heap& m_heap;
        Prototype* m_proto = nullptr;

        uint8_t m_target = 0;      
        size_t m_freeReg = 0;
        size_t m_firstTemp = 0; 
        uint32_t m_errorCount = 0;
        bool m_registersReported = false;  

        std::vector<LocalVar> m_locals;    // variables visibles, la plus récente à la fin
        std::vector<Scope> m_scopes;    // un élément par bloc ouvert, vide = niveau global

        void CompileTo(Node& _node, uint8_t _dst);
        LocalVar* FindLocal(std::string const& _name);

        uint8_t AllocReg(Node const& _at);
        bool IsTopTemp(uint8_t _reg) const;

        size_t Emit(Instruction _i, Node const& _at);
        uint16_t ConstantIndex(Value const& _v, Node const& _at);

        void Report(Node const& _at, std::string const& _message);
    };
}

#endif
