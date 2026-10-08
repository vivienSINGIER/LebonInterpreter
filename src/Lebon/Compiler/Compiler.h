#ifndef COMPILER_COMPILER_H_DEFINED
#define COMPILER_COMPILER_H_DEFINED

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../Bytecode/Heap.hpp"
#include "../Bytecode/Prototype.hpp"
#include "../Parser/AST.h"

namespace Bytecode
{
    struct LocalVar
    {
        std::string name;
        uint8_t registre;
        bool captured = false;   // vrai si une fonction interne la capture : un appel peut alors la modifier
    };

    struct Scope
    {
        size_t localCount;
        size_t firstTemp;
    };

    // Tout ce qu'il faut pour compiler une fonction. Le compilateur en empile un par fonction ouverte,
    // le premier est toujours le main
    struct FuncState
    {
        Prototype* proto = nullptr;

        size_t freeReg = 0;
        size_t firstTemp = 0;

        std::vector<LocalVar> locals;           // variables visibles, la plus recente a la fin
        std::vector<Scope> scopes;              // un element par bloc ouvert dans la fonction
        std::vector<std::string> upvalueNames;  // meme taille que proto->upvalues
    };

    class Compiler : public Visitor
    {
    public:
        explicit Compiler(Heap& _heap) : m_heap(_heap) {}

        // Renvoie la fonction main et la table des globales. Resultat vide (main == nullptr) si erreur.
        // Le programme doit avoir passe l'analyse semantique sans erreur
        CompiledProgram Compile(Program& _program);

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
        void Visit(IfStmt& _node) override;
        void Visit(Block& _node) override;
        void Visit(FuncDecl& _node) override;
        void Visit(Program& _node) override;

    private:
        Heap& m_heap;
        Semantics::SymbolTable* m_symbols = nullptr;   // table du programme en cours de compilation

        std::deque<FuncState> m_funcs;     // deque : les references restent valides quand on empile
        uint8_t m_target = 0;
        uint32_t m_errorCount = 0;
        bool m_registersReported = false;
          
        FuncState& Func() { return m_funcs.back(); }
        Prototype& Proto() { return *m_funcs.back().proto; }

        // Vrai si une variable declaree ici est une globale (main, hors de tout bloc)
        bool IsGlobalScope();

        void CompileTo(Node& _node, uint8_t _dst);
        std::unique_ptr<Prototype> CompileFunction(FuncDecl& _node);
        void CompileAssign(AssignExpr& _node, bool _wantValue);

        LocalVar* FindLocal(FuncState& _fn, std::string const& _name);

        // La variable locale de la fonction courante désignée par l'expression, nullptr si ce n'en est pas une.
        // Sa valeur est déjà dans un registre : on le lit sur place au lieu de la recopier dans un temporaire
        LocalVar* LocalOf(Expr& _expr);

        // Vrai si évaluer l'expression peut changer la variable : une affectation, ou un appel si une fermeture la capture
        bool MayChange(Node const& _expr, LocalVar const& _local) const;

        // Index de l'upvalue de la fonction _level, -1 si le nom n'est pas une variable d'une fonction parente
        int ResolveUpvalue(size_t _level, std::string const& _name, Node const& _at);

        uint8_t AllocReg(Node const& _at);
        bool IsTopTemp(uint8_t _reg);

        size_t Emit(Instruction _i, Node const& _at);
        uint16_t ConstantIndex(Value const& _v, Node const& _at);
        // Index de la constante d'un littéral nombre ou chaîne s'il tient dans l'opérande C (0 à 255), sinon -1
        int ConstantOperand(Expr& _expr);
        uint16_t GlobalSlot(Semantics::SymbolId _id, Node const& _at);
        void PatchJumpHere(size_t _jump, Node const& _at);

        void Report(Node const& _at, std::string const& _message);
    };
}

#endif
