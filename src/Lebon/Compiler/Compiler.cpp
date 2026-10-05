#include "Compiler.h"

#include <charconv>

#include "../core/Error.h"

namespace Bytecode
{
    std::unique_ptr<Prototype> Compiler::Compile(Program& _program)
    {
        auto main = std::make_unique<Prototype>();

        m_proto = main.get();
        m_freeReg = 0;
        m_firstTemp = 0;
        m_errorCount = 0;
        m_registersReported = false;
        
        m_locals.clear();
        m_scopes.clear();

        _program.Accept(*this);

        m_proto = nullptr;
        if (m_errorCount > 0)
            return nullptr;

        return main;
    }

    void Compiler::CompileTo(Node& _node, uint8_t _dst)
    {
        m_target = _dst;
        _node.Accept(*this);
    }

    LocalVar* Compiler::FindLocal(std::string const& _name)
    {
        for (size_t i = m_locals.size(); i > 0; --i)
        {
            if (m_locals[i - 1].name == _name)
                return &m_locals[i - 1];
        }
        return nullptr;
    }

    uint8_t Compiler::AllocReg(Node const& _at)
    {
        if (m_freeReg >= MaxRegisters)
        {
            if (m_registersReported == false)
                Report(_at, "expression too complex, a function can use at most 256 registers");

            m_registersReported = true;
            return static_cast<uint8_t>(MaxRegisters - 1);
        }

        uint8_t reg = static_cast<uint8_t>(m_freeReg++);
        if (m_freeReg > m_proto->maxRegisters)
            m_proto->maxRegisters = static_cast<uint16_t>(m_freeReg);
        return reg;
    }

    bool Compiler::IsTopTemp(uint8_t _register) const
    {
        return _register >= m_firstTemp && static_cast<size_t>(_register) + 1 == m_freeReg;
    }

    size_t Compiler::Emit(Instruction _i, Node const& _at)
    {
        return m_proto->Emit(_i, _at.row);
    }

    uint16_t Compiler::ConstantIndex(Value const& _value, Node const& _at)
    {
        int32_t index = m_proto->AddConstant(_value);
        if (index < 0)
        {
            Report(_at, "too many constants in one function");
            return 0;
        }
        return static_cast<uint16_t>(index);
    }

    void Compiler::Report(Node const& _at, std::string const& _message)
    {
        ErrorManager::LogError(Error::Execution(_message, _at.row, _at.column));
        m_errorCount++;
    }

    // Expressions
    void Compiler::Visit(NumberLiteral& _node)
    {
        double number = 0.0;

        char const* first = _node.litteral.data();
        char const* last = first + _node.litteral.size();

        auto result = std::from_chars(first, last, number);

        if (result.ec != std::errc() || result.ptr != last)
        {
            Report(_node, "invalid number '" + _node.litteral + "'");
            return;
        }

        uint16_t k = ConstantIndex(Value::MakeNumber(number), _node);
        Emit(EncodeABx(OpCode::LoadK, m_target, k), _node);
    }

    void Compiler::Visit(StringLiteral& _node)
    {
        uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.value)), _node);
        Emit(EncodeABx(OpCode::LoadK, m_target, k), _node);
    }

    void Compiler::Visit(BooleanLiteral& _node)
    {
        Emit(EncodeABC(OpCode::LoadBool, m_target, _node.value ? 1 : 0), _node);
    }

    void Compiler::Visit(Identifier& _node)
    {
        if (LocalVar* local = FindLocal(_node.name))
        {
            if (local->registre != m_target)
                Emit(EncodeABC(OpCode::Move, m_target, local->registre), _node);
            return;
        }

        uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.name)), _node);
        Emit(EncodeABx(OpCode::GetGlobal, m_target, k), _node);
    }

    void Compiler::Visit(UnaryExpr& _node)
    {
        if (_node.op != TokenType::SUB)
        {
            Report(_node, "unsupported unary operator");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = m_freeReg;

        uint8_t operand = dst;
        if (IsTopTemp(dst) == false)
            operand = AllocReg(_node);

        CompileTo(*_node.operand, operand);
        Emit(EncodeABC(OpCode::Neg, dst, operand), _node);

        m_freeReg = saved;
    }

    void Compiler::Visit(BinaryExpr& _node)
    {
        OpCode op = OpCode::Add;
        switch (_node.op)
        {
        case TokenType::ADD:
            op = OpCode::Add; break;
        case TokenType::SUB:
            op = OpCode::Sub; break;
        case TokenType::MUL: 
            op = OpCode::Mul; break;
        case TokenType::DIV: 
            op = OpCode::Div; break;
        default:
            Report(_node, "unsupported binary operator");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = m_freeReg;

        uint8_t left = dst;
        if (IsTopTemp(dst) == false)
            left = AllocReg(_node);
        CompileTo(*_node.left, left);

        uint8_t right = AllocReg(_node);
        CompileTo(*_node.right, right);

        Emit(EncodeABC(op, dst, left, right), _node);

        m_freeReg = saved;
    }

    void Compiler::Visit(CallExpr& _node)
    {
        if (_node.args.size() >= MaxRegisters)
        {
            Report(_node, "too many arguments in a call");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = m_freeReg;

        uint8_t base = dst;
        if (IsTopTemp(dst) == false)
            base = AllocReg(_node);

        CompileTo(*_node.callee, base);
        for (ExprPtr& arg : _node.args)
        {
            uint8_t reg = AllocReg(*arg);
            CompileTo(*arg, reg);
        }

        Emit(EncodeABC(OpCode::Call, base, static_cast<uint8_t>(_node.args.size())), _node);
        if (base != dst)
            Emit(EncodeABC(OpCode::Move, dst, base), _node);

        m_freeReg = saved;
    }

    // Statements
    void Compiler::Visit(ExprStmt& _node)
    {
        size_t saved = m_freeReg;

        uint8_t reg = AllocReg(_node);
        CompileTo(*_node.expr, reg);

        m_freeReg = saved;
    }

    void Compiler::Visit(Program& _node)
    {
        for (NodePtr& statement : _node.statements)
            statement->Accept(*this);

        uint32_t lastRow = m_proto->rows.empty() ? 0 : m_proto->rows.back();
        m_proto->Emit(EncodeABC(OpCode::Return, 0, 0), lastRow);
    }

    // Variables
    void Compiler::Visit(VarDecl& _node)
    {
        size_t saved = m_freeReg;

        uint8_t reg = AllocReg(_node);

        if (_node.init)
            CompileTo(*_node.init, reg);
        else
            Emit(EncodeABC(OpCode::LoadNil, reg), _node);

        if (m_scopes.empty())
        {
            uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.name)), _node);
            Emit(EncodeABx(OpCode::SetGlobal, reg, k), _node);
            m_freeReg = saved;
        }
        else
        {
            m_locals.push_back({ _node.name, reg });
            m_firstTemp = m_freeReg;
        }
    }

    void Compiler::Visit(AssignExpr& _node)
    {
        Report(_node, "assignments cannot be compiled yet");
    }

    void Compiler::Visit(ReturnStmt& _node)
    {
        Report(_node, "return cannot be compiled yet");
    }

    void Compiler::Visit(Block& _node)
    {
        Report(_node, "blocks cannot be compiled yet");
    }

    void Compiler::Visit(FuncDecl& _node)
    {
        Report(_node, "functions cannot be compiled yet");
    }
}
