#include "CodeGen.h"

#include <algorithm>
#include <bit>

Jit::CodeGen::CodeGen(JitCode& _jit, Semantics::SymbolTable& _table) : m_jit(_jit), m_table(_table)
{
    
}

bool Jit::CodeGen::Run(Program& _program)
{
    _program.Accept(*this);
    
    m_jit.code.Load(m_asm.Code());

    return m_errCount == 0;
}

void Jit::CodeGen::Visit(Program& _p)
{
    m_jit.globals.resize(m_table.globalCount);
    
    // Visit 
    for (auto& f : _p.statements)
    {
        FuncDecl* funcDecl = dynamic_cast<FuncDecl*>(f.get());
        if (funcDecl == nullptr)
            continue;
        
        f->Accept(*this);
    }
    
    m_jit.entry = m_asm.Size();
    OpenFrame();
    
    for (auto& f : _p.statements)
    {
        FuncDecl* funcDecl = dynamic_cast<FuncDecl*>(f.get());
        if (funcDecl != nullptr)
            continue;
        f->Accept(*this);
    }
    
    CloseFrame();
}

void Jit::CodeGen::Visit(VarDecl& _varDecl)
{
}

void Jit::CodeGen::Visit(Identifier& _identifier)
{
}

void Jit::CodeGen::Visit(AssignExpr& _expr)
{
}

void Jit::CodeGen::Visit(Block& _b)
{
}

void Jit::CodeGen::Visit(NumberLiteral& _l)
{
    m_asm.MovEaxImm32(std::bit_cast<uint32_t>(_l.value));
    m_asm.MovdXmm0Eax();
}

void Jit::CodeGen::Visit(StringLiteral& _l)
{
}

void Jit::CodeGen::Visit(BooleanLiteral& _l)
{
}

void Jit::CodeGen::Visit(ExprStmt& _expr)
{
    _expr.expr->Accept(*this);
}

void Jit::CodeGen::Visit(FuncDecl& _func)
{
    OpenFrame();
    
    for (auto& n : _func.body->statements)
    {
        n->Accept(*this);
    }
    
    CloseFrame();
}

void Jit::CodeGen::Visit(CallExpr& _call)
{
}

void Jit::CodeGen::Visit(ReturnStmt& _rtrn)
{
}

void Jit::CodeGen::Visit(UnaryExpr& _expr)
{
}

void Jit::CodeGen::Visit(BinaryExpr& _expr)
{
}

void Jit::CodeGen::OpenFrame()
{
    ResetFrame();
    m_asm.PushRbp();
    m_asm.MovRbpRsp();

    // The size isn't known before the body is compiled, CloseFrame writes it over these 4 bytes
    m_asm.SubRspImm32(0);
    m_frame.framePatch = m_asm.Size() - 4;
}

void Jit::CodeGen::CloseFrame()
{
    m_asm.MovRspRbp();
    m_asm.PopRbp();
    m_asm.Ret();

    m_asm.Patch32(m_frame.framePatch, GetFrameSize());
}

void Jit::CodeGen::ResetFrame()
{
    m_frame.usedSlots = 0;
    m_frame.framePatch = 0;
    m_frame.maxArgs = 0;
    m_frame.maxSlots = 0;
}

int32_t Jit::CodeGen::TakeSlot()
{
    m_frame.usedSlots++;
    m_frame.maxSlots = std::max(m_frame.maxSlots, m_frame.usedSlots);
    return -8 * static_cast<int32_t>(m_frame.usedSlots);
}

void Jit::CodeGen::ReleaseSlot()
{
    m_frame.usedSlots--;
}

uint32_t Jit::CodeGen::GetFrameSize()
{
    uint32_t frameSize = 8 * m_frame.maxSlots + std::max(32u, 8 * m_frame.maxArgs);
    frameSize = (frameSize + 15) & ~15u; // round up to a multiple of 16;
    return frameSize;
}


