#include "CodeGen.h"

#include <algorithm>
#include <bit>

#include "Runtime.hpp"

Jit::CodeGen::CodeGen(JitCode& _jit, Semantics::SymbolTable& _table) : m_jit(_jit), m_table(_table)
{
    
}

bool Jit::CodeGen::Run(Program& _program)
{
    Runtime::g_heap = &m_jit.heap;
    _program.Accept(*this);
    
    m_jit.code.Load(m_asm.Code());

    return m_errCount == 0;
}

void Jit::CodeGen::Visit(Program& _p)
{
    m_jit.globals.resize(m_table.globalCount);
    m_functionOffsets.assign(m_table.symbols.size(), 0);
    m_functionDepths.assign(m_table.symbols.size(), 0);
    m_frameOffsets.assign(m_table.symbols.size(), 0);
    m_frameDepths.assign(m_table.symbols.size(), 0);
    
    m_depth = 0;
    m_jit.entry = m_asm.Size();
    OpenFrame();

    for (auto& s : _p.statements)
        s->Accept(*this);

    CloseFrame();
    
    for (size_t i = 0; i < m_pending.size(); i++)
        CompileFunction(*m_pending[i]);
    
    for (CallSite const& call : m_calls)
    {
        int64_t end = static_cast<int64_t>(call.patch) + 4;
        int64_t distance = static_cast<int64_t>(m_functionOffsets[call.function]) - end;
        m_asm.Patch32(call.patch, static_cast<uint32_t>(distance));
    }
}

void Jit::CodeGen::Visit(VarDecl& _varDecl)
{
    if (_varDecl.init != nullptr)
        _varDecl.init->Accept(*this);
    
    SymbolInfo& s = m_table.Get(_varDecl.symbol);
    if (s.type == SymbolType::LocalVariable)
    {
        m_frameOffsets[_varDecl.symbol] = TakeSlot();
        m_frameDepths[_varDecl.symbol] = m_depth;
    }

    if (_varDecl.init != nullptr)
        Store(_varDecl.symbol);
}

void Jit::CodeGen::Visit(Identifier& _identifier)
{
    Load(_identifier.symbol);
}

void Jit::CodeGen::Visit(AssignExpr& _expr)
{
    _expr.value->Accept(*this);
    Store(_expr.symbol);
}

void Jit::CodeGen::Visit(Block& _b)
{
    uint32_t usedSlot = m_frame.usedSlots;
    for (auto& s : _b.statements)
        s->Accept(*this);
    m_frame.usedSlots = usedSlot;
}

void Jit::CodeGen::Visit(NumberLiteral& _l)
{
    m_asm.MovEaxImm32(std::bit_cast<uint32_t>(_l.value));
    m_asm.MovdXmm0Eax();
}

void Jit::CodeGen::Visit(StringLiteral& _l)
{
    Bytecode::StringObj* p = m_jit.heap.Intern(_l.value);
    m_asm.MovRaxImm64(std::bit_cast<int64_t>(p));
}

void Jit::CodeGen::Visit(BooleanLiteral& _l)
{
    uint32_t v = _l.value ? 1 : 0;
    m_asm.MovEaxImm32(v);
}

void Jit::CodeGen::Visit(ExprStmt& _expr)
{
    _expr.expr->Accept(*this);
}

void Jit::CodeGen::Visit(FuncDecl& _func)
{
    m_functionDepths[_func.symbol] = m_depth + 1;
    m_pending.push_back(&_func);
}

void Jit::CodeGen::CompileFunction(FuncDecl& _func)
{
    SymbolId s = _func.symbol;
    m_depth = m_functionDepths[s];
    m_functionOffsets[s] = m_asm.Size();
    
    for (size_t i = 0; i < _func.params.size(); i++)
    {
        SymbolId sp = _func.params[i].symbol;
        m_frameOffsets[sp] = 24 + 8 * static_cast<int>(i);
        m_frameDepths[sp] = m_depth;
    }

    OpenFrame();

    for (auto& n : _func.body->statements)
    {
        n->Accept(*this);
    }

    CloseFrame();
}

void Jit::CodeGen::FramePointer(uint32_t _hops)
{
    m_asm.MovRcxRbpReg();
    for (uint32_t i = 0; i < _hops; i++)
        m_asm.MovRcxArcxOff(16);
}

void Jit::CodeGen::Visit(CallExpr& _call)
{
    SymbolInfo const& function = m_table.Get(_call.symbol);

    if (function.isBuiltIn)
    {
        CallBuiltIn(_call, function);
        return;
    }

    std::vector<int32_t> slots;
    for (auto& arg : _call.args)
    {
        arg->Accept(*this);
        slots.push_back(TakeSlot());
        if (arg->type == InferredType::Number)
            m_asm.MovssRbpXmm0(slots.back());
        else
            m_asm.MovRbpRax(slots.back());
    }
    
    for (size_t i = 0; i < slots.size(); i++)
    {
        m_asm.MovRaxRbp(slots[i]);
        m_asm.MovRspRax(static_cast<uint32_t>(8 * (i + 1)));
    }
    
    uint32_t parentDepth = m_functionDepths[_call.symbol] - 1;
    FramePointer(m_depth - parentDepth);
    m_asm.MovRspRcx(0);

    m_asm.Callr32(0);
    m_calls.push_back({ m_asm.Size() - 4, _call.symbol });

    for (size_t i = 0; i < slots.size(); i++)
        ReleaseSlot();

    m_frame.maxArgs = std::max(m_frame.maxArgs, static_cast<uint32_t>(_call.args.size() + 1));
}

void Jit::CodeGen::Visit(ReturnStmt& _rtrn)
{
    if (_rtrn.value != nullptr)
        _rtrn.value->Accept(*this);
    
    m_asm.MovRspRbp();
    m_asm.PopRbp();
    m_asm.Ret();
}

void Jit::CodeGen::Visit(UnaryExpr& _expr)
{
    if (_expr.op == TokenType::SUB)
    {
        _expr.operand->Accept(*this);
        m_asm.MovapsX1X0();
        
        m_asm.Xorps();
        m_asm.Subss();
    }
}

void Jit::CodeGen::Visit(BinaryExpr& _expr)
{
    if (_expr.type == Semantics::InferredType::Number)
    {
        StoreNums(_expr);
        
        NumberLiteral* divisor = dynamic_cast<NumberLiteral*>(_expr.right.get());
        if (_expr.op == TokenType::DIV && divisor != nullptr && divisor->value == 0.0f)
            Report(*divisor, "division by zero");

        switch (_expr.op)
        {
        case TokenType::ADD:
            m_asm.Adds(); break;
        case TokenType::SUB:
            m_asm.Subss(); break;
        case TokenType::MUL:
            m_asm.Mulss(); break;
        case TokenType::DIV:
            m_asm.Divss(); break;
        default: break;
        }
        return;
    }
    
    if (_expr.type == Semantics::InferredType::String)
    {
        _expr.left->Accept(*this);
        int32_t slot = TakeSlot();
        m_asm.MovRbpRax(slot);
        
        _expr.right->Accept(*this);
        m_asm.MovRdxRax();
        m_asm.MovRcxRbp(slot);
        ReleaseSlot();

        switch (_expr.op)
        {
        case TokenType::ADD:
            CallHelper(&Runtime::Concat);    
        default: break;
        }
        return;
    }
    
    if (_expr.left->type == InferredType::Number)
    {
        StoreNums(_expr);
        
        m_asm.CmpX0X1();

        switch (_expr.op)
        {
        case TokenType::EQ:
            m_asm.SeteAl(); break;
        case TokenType::NEQ:
            m_asm.SetneAl(); break;
        case TokenType::LT:
            m_asm.SetlAl(); break;
        case TokenType::GT:
            m_asm.SetgAl(); break;
        case TokenType::LE:
            m_asm.SetleAl(); break;
        case TokenType::GE:
            m_asm.SetgeAl(); break;
        default: break;
        }
        
        m_asm.MovzxEaxAl();
        return;
    }
    
    if (_expr.left->type == InferredType::String || _expr.left->type == InferredType::Bool)
    {
        _expr.left->Accept(*this);
        int32_t slot = TakeSlot();
        m_asm.MovRbpRax(slot);
        
        _expr.right->Accept(*this);
        m_asm.CmpRaxRbp(slot);
        ReleaseSlot();

        switch (_expr.op)
        {
        case TokenType::EQ:
            m_asm.SeteAl(); break;
        case TokenType::NEQ:
            m_asm.SetneAl(); break;
        default: break;
        }
        
        m_asm.MovzxEaxAl();
        return;
    }
}

void Jit::CodeGen::Visit(IfStmt& _expr)
{
    _expr.condition->Accept(*this);
    
    m_asm.TestEaxEax();
    m_asm.JzRel32(0);
    size_t jz = m_asm.Size() - 4;
    
    _expr.thenBranch->Accept(*this);
    
    if (_expr.elseBranch != nullptr)
    {
        m_asm.JmpRel32(0);
        size_t jmp = m_asm.Size() - 4;
        m_asm.Patch32(jz, static_cast<uint32_t>(m_asm.Size() - (jz + 4)));
        _expr.elseBranch->Accept(*this);
        m_asm.Patch32(jmp, static_cast<uint32_t>(m_asm.Size() - (jmp + 4)));
    }
    else
        m_asm.Patch32(jz, static_cast<uint32_t>(m_asm.Size() - (jz + 4)));
}

void Jit::CodeGen::CallBuiltIn(CallExpr& _call, SymbolInfo const& _function)
{
    if (_function.name != "afise" || _call.args.size() != 1)
    {
        Report(_call, "built-in '" + _function.name + "' isn't supported");
        return;
    }
    
    Expr& arg = *_call.args[0];
    arg.Accept(*this);

    switch (arg.type)
    {
    case InferredType::Number:
        CallHelper(&Runtime::PrintNumber);
        break;
    case InferredType::String:
        m_asm.MovRcxRax();
        CallHelper(&Runtime::PrintString);
        break;
    case InferredType::Bool:
        m_asm.MovRcxRax();
        CallHelper(&Runtime::PrintBool);
        break;
    default:
        Report(arg, "this value can't be printed");
        break;
    }
}

void Jit::CodeGen::StoreNums(BinaryExpr& _expr)
{
    _expr.left->Accept(*this);
    int32_t slot = TakeSlot();
    m_asm.MovssRbpXmm0(slot);
        
    _expr.right->Accept(*this);
    m_asm.MovapsX1X0();
    m_asm.MovssXmm0Rbp(slot);
    ReleaseSlot();
}

void Jit::CodeGen::Report(Node const& _at, std::string const& _message)
{
    Report(_at.row, _at.column, _message);
}

void Jit::CodeGen::Report(uint32_t _row, uint32_t _column, std::string const& _message)
{
    ErrorManager::LogError(Error::Execution(_message, _row, _column));
    m_errCount++;
}

void Jit::CodeGen::OpenFrame()
{
    ResetFrame();
    m_asm.PushRbp();
    m_asm.MovRbpRsp();
    
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

void Jit::CodeGen::Load(SymbolId _symbolId)
{
    SymbolInfo const& symbol = m_table.Get(_symbolId);
    bool isNumber = symbol.infType == InferredType::Number;

    switch (symbol.type)
    {
    case SymbolType::GlobalVariable:
        m_asm.MovRcxImm64(GlobalAddress(symbol));
        if (isNumber)
            m_asm.MovssXmm0Arcx();
        else
            m_asm.MovRaxArcx();
        break;
    case SymbolType::LocalVariable:
    case SymbolType::Param:
    {
        int32_t offset = m_frameOffsets[_symbolId];
        uint32_t hops = m_depth - m_frameDepths[_symbolId];
        
        if (hops == 0)
        {
            if (isNumber)
                m_asm.MovssXmm0Rbp(offset);
            else
                m_asm.MovRaxRbp(offset);
            break;
        }

        FramePointer(hops);
        if (isNumber)
            m_asm.MovssXmm0ArcxOff(offset);
        else
            m_asm.MovRaxArcxOff(offset);
        break;
    }
    default: break;
    }
}

void Jit::CodeGen::Store(SymbolId _symbolId)
{
    SymbolInfo const& symbol = m_table.Get(_symbolId);
    bool isNumber = symbol.infType == InferredType::Number;

    switch (symbol.type)
    {
    case SymbolType::GlobalVariable:
        m_asm.MovRcxImm64(GlobalAddress(symbol));
        if (isNumber)
            m_asm.MovssArcxXmm0();
        else
            m_asm.MovArcxRax();
        break;
    case SymbolType::LocalVariable:
    case SymbolType::Param:
    {
        int32_t offset = m_frameOffsets[_symbolId];
        uint32_t hops = m_depth - m_frameDepths[_symbolId];

        if (hops == 0)
        {
            if (isNumber)
                m_asm.MovssRbpXmm0(offset);
            else
                m_asm.MovRbpRax(offset);
            break;
        }

        FramePointer(hops);
        if (isNumber)
            m_asm.MovssArcxOffXmm0(offset);
        else
            m_asm.MovArcxOffRax(offset);
        break;
    }
    default: break;
    }
}

uint64_t Jit::CodeGen::GlobalAddress(SymbolInfo const& _symbol)
{
    return reinterpret_cast<uint64_t>(&m_jit.globals[_symbol.globalSlot]);
}

