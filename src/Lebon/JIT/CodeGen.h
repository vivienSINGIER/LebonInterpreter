#ifndef JIT_CODE_GEN_H_DEFINED
#define JIT_CODE_GEN_H_DEFINED

#include <string>

#include "Assembler.h"
#include "Jit.hpp"
#include "Parser/AST.h"
#include "core/Error.h"

using namespace Semantics;

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
        CodeGen(JitCode& _jit, SymbolTable& _table);
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
        SymbolTable& m_table;
        Assembler m_asm;
        FrameState m_frame;
        
        size_t m_errCount = 0;
        
        std::vector<size_t> m_functionOffsets;
        std::vector<uint32_t> m_functionDepths;
        std::vector<int32_t> m_frameOffsets;
        std::vector<uint32_t> m_frameDepths;
        
        uint32_t m_depth = 0;
        
        std::vector<FuncDecl*> m_pending;

        struct CallSite
        {
            size_t patch;
            SymbolId function;
        };
        std::vector<CallSite> m_calls;

        void Report(Node const& _at, std::string const& _message);
        void Report(uint32_t _row, uint32_t _column, std::string const& _message);

        void ResetFrame();
        void OpenFrame();
        void CloseFrame();
        
        int32_t TakeSlot();
        void ReleaseSlot();
        uint32_t GetFrameSize();
        
        void Load(SymbolId _symbolId);
        void Store(SymbolId _symbolId);
        uint64_t GlobalAddress(SymbolInfo const& _symbol);

        void CompileFunction(FuncDecl& _func);
        void FramePointer(uint32_t _hops);

        void CallBuiltIn(CallExpr& _call, SymbolInfo const& _function);
        
        template <typename Fn>
        void CallHelper(Fn* _helper)
        {
            m_asm.MovRaxImm64(reinterpret_cast<uint64_t>(_helper));
            m_asm.CallRax();
        }
    };
    
}

#endif

