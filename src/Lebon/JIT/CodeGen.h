#ifndef JIT_CODE_GEN_H_DEFINED
#define JIT_CODE_GEN_H_DEFINED

#include "Assembler.h"
#include "Jit.hpp"
#include "Parser/AST.h"

namespace Jit
{
 
    struct FrameState
    {
        size_t framePatch = 0;
        uint32_t usedSlots = 0;
        uint32_t maxSlots = 0;
        uint32_t maxArgs = 0;
    };
    
    class CodeGen : public Visitor
    {
    public:
        CodeGen(JitCode& _jit, Semantics::SymbolTable& _table);
        ~CodeGen() = default;
        
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
        void Visit(UnaryExpr& _expr) override;
        void Visit(BinaryExpr& _expr) override;
        
    private:
        JitCode& m_jit;
        Semantics::SymbolTable& m_table;
        Assembler m_asm;
        FrameState m_frame;
        
        size_t m_errCount = 0;
        
        std::vector<size_t> m_functionOffsets; 
        std::vector<int32_t> m_frameOffsets;
        
        void ResetFrame();
        void OpenFrame();
        void CloseFrame();
        
        int32_t TakeSlot();
        void ReleaseSlot();
        uint32_t GetFrameSize();
    };
    
}

#endif

