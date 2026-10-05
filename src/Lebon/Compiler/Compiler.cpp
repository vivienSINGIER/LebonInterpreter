#include "Compiler.h"

#include <charconv>

#include "../core/Error.h"

namespace Bytecode
{
    std::unique_ptr<Prototype> Compiler::Compile(Program& _program)
    {
        auto main = std::make_unique<Prototype>();

        m_funcs.clear();
        m_funcs.emplace_back();
        Fn().proto = main.get();

        m_target = 0;
        m_errorCount = 0;
        m_registersReported = false;

        _program.Accept(*this);

        m_funcs.clear();
        if (m_errorCount > 0)
            return nullptr;

        return main;
    }

    bool Compiler::IsGlobalScope() const
    {
        return m_funcs.size() == 1 && Fn().scopes.empty();
    }

    void Compiler::CompileTo(Node& _node, uint8_t _dst)
    {
        m_target = _dst;
        _node.Accept(*this);
    }

    LocalVar* Compiler::FindLocal(FuncState& _fn, std::string const& _name)
    {
        for (size_t i = _fn.locals.size(); i > 0; --i)
        {
            if (_fn.locals[i - 1].name == _name)
                return &_fn.locals[i - 1];
        }
        return nullptr;
    }

    int Compiler::ResolveUpvalue(size_t _level, std::string const& _name, Node const& _at)
    {
        if (_level == 0)
            return -1;

        FuncState& fn = m_funcs[_level];
        for (size_t i = 0; i < fn.upvalueNames.size(); ++i)
        {
            if (fn.upvalueNames[i] == _name)
                return static_cast<int>(i);
        }

        UpvalDesc desc;
        if (LocalVar* local = FindLocal(m_funcs[_level - 1], _name))
        {
            desc.fromParentRegister = true;
            desc.index = local->registre;
        }
        else
        {
            int parentIndex = ResolveUpvalue(_level - 1, _name, _at);
            if (parentIndex < 0)
                return -1;

            desc.fromParentRegister = false;
            desc.index = static_cast<uint8_t>(parentIndex);
        }

        if (fn.upvalueNames.size() >= MaxRegisters)
        {
            Report(_at, "too many upvalues in one function");
            return 0;
        }

        fn.upvalueNames.push_back(_name);
        fn.proto->upvalues.push_back(desc);
        return static_cast<int>(fn.upvalueNames.size() - 1);
    }

    uint8_t Compiler::AllocReg(Node const& _at)
    {
        FuncState& fn = Fn();
        if (fn.freeReg >= MaxRegisters)
        {
            if (m_registersReported == false)
                Report(_at, "expression too complex, a function can use at most 256 registers");

            m_registersReported = true;
            return static_cast<uint8_t>(MaxRegisters - 1);
        }

        uint8_t reg = static_cast<uint8_t>(fn.freeReg++);
        if (fn.freeReg > fn.proto->maxRegisters)
            fn.proto->maxRegisters = static_cast<uint16_t>(fn.freeReg);
        return reg;
    }

    bool Compiler::IsTopTemp(uint8_t _register) const
    {
        FuncState const& fn = Fn();
        return _register >= fn.firstTemp && static_cast<size_t>(_register) + 1 == fn.freeReg;
    }

    size_t Compiler::Emit(Instruction _i, Node const& _at)
    {
        return Proto().Emit(_i, _at.row);
    }

    uint16_t Compiler::ConstantIndex(Value const& _value, Node const& _at)
    {
        int32_t index = Proto().AddConstant(_value);
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
        if (LocalVar* local = FindLocal(Fn(), _node.name))
        {
            if (local->registre != m_target)
                Emit(EncodeABC(OpCode::Move, m_target, local->registre), _node);
            return;
        }

        int upvalue = ResolveUpvalue(m_funcs.size() - 1, _node.name, _node);
        if (upvalue >= 0)
        {
            Emit(EncodeABC(OpCode::GetUpval, m_target, static_cast<uint8_t>(upvalue)), _node);
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
        size_t saved = Fn().freeReg;

        uint8_t operand = dst;
        if (IsTopTemp(dst) == false)
            operand = AllocReg(_node);

        CompileTo(*_node.operand, operand);
        Emit(EncodeABC(OpCode::Neg, dst, operand), _node);

        Fn().freeReg = saved;
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
        size_t saved = Fn().freeReg;

        uint8_t left = dst;
        if (IsTopTemp(dst) == false)
            left = AllocReg(_node);
        CompileTo(*_node.left, left);

        uint8_t right = AllocReg(_node);
        CompileTo(*_node.right, right);

        Emit(EncodeABC(op, dst, left, right), _node);

        Fn().freeReg = saved;
    }

    void Compiler::Visit(CallExpr& _node)
    {
        if (_node.args.size() >= MaxRegisters)
        {
            Report(_node, "too many arguments in a call");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = Fn().freeReg;

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

        Fn().freeReg = saved;
    }

    void Compiler::Visit(AssignExpr& _node)
    {
        CompileAssign(_node, true);
    }

    // Stores the value in the variable. _wantValue : the result of the expression must also end up in m_target
    void Compiler::CompileAssign(AssignExpr& _node, bool _wantValue)
    {
        uint8_t dst = m_target;
        size_t saved = Fn().freeReg;

        if (LocalVar* local = FindLocal(Fn(), _node.name))
        {
            uint8_t reg = local->registre;

            // The value is built straight in the variable's register
            CompileTo(*_node.value, reg);
            if (_wantValue && dst != reg)
                Emit(EncodeABC(OpCode::Move, dst, reg), _node);
            return;
        }

        // Globals and upvalues are written from a register : the caller's, or a temporary if nobody wants the value
        uint8_t valueReg = _wantValue ? dst : AllocReg(_node);
        CompileTo(*_node.value, valueReg);

        int upvalue = ResolveUpvalue(m_funcs.size() - 1, _node.name, _node);
        if (upvalue >= 0)
        {
            Emit(EncodeABC(OpCode::SetUpval, valueReg, static_cast<uint8_t>(upvalue)), _node);
        }
        else
        {
            uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.name)), _node);
            Emit(EncodeABx(OpCode::SetGlobal, valueReg, k), _node);
        }

        Fn().freeReg = saved;
    }

    // Statements
    void Compiler::Visit(ExprStmt& _node)
    {
        size_t saved = Fn().freeReg;

        if (auto* assign = dynamic_cast<AssignExpr*>(_node.expr.get()))
        {
            // Nobody reads the result of an assignment used as a statement
            CompileAssign(*assign, false);
        }
        else
        {
            uint8_t reg = AllocReg(_node);
            CompileTo(*_node.expr, reg);
        }

        Fn().freeReg = saved;
    }

    void Compiler::Visit(Program& _node)
    {
        for (NodePtr& statement : _node.statements)
            statement->Accept(*this);

        uint32_t lastRow = Proto().rows.empty() ? 0 : Proto().rows.back();
        Proto().Emit(EncodeABC(OpCode::Return, 0, 0), lastRow);
    }

    // Variables
    void Compiler::Visit(VarDecl& _node)
    {
        FuncState& fn = Fn();
        size_t saved = fn.freeReg;

        uint8_t reg = AllocReg(_node);

        if (_node.init)
            CompileTo(*_node.init, reg);
        else
            Emit(EncodeABC(OpCode::LoadNil, reg), _node);

        if (IsGlobalScope())
        {
            uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.name)), _node);
            Emit(EncodeABx(OpCode::SetGlobal, reg, k), _node);
            fn.freeReg = saved;
        }
        else
        {
            fn.locals.push_back({ _node.name, reg });
            fn.firstTemp = fn.freeReg;
        }
    }

    void Compiler::Visit(ReturnStmt& _node)
    {
        if (_node.value == nullptr)
        {
            Emit(EncodeABC(OpCode::Return, 0, 0), _node);
            return;
        }

        size_t saved = Fn().freeReg;

        uint8_t reg = AllocReg(_node);
        CompileTo(*_node.value, reg);
        Emit(EncodeABC(OpCode::Return, reg, 1), _node);

        Fn().freeReg = saved;
    }

    void Compiler::Visit(Block& _node)
    {
        FuncState& fn = Fn();
        fn.scopes.push_back({ fn.locals.size(), fn.firstTemp });

        for (NodePtr& statement : _node.statements)
            statement->Accept(*this);

        // The locals of the block disappear and their registers are free again
        Scope scope = fn.scopes.back();
        fn.scopes.pop_back();
        fn.locals.resize(scope.localCount);
        fn.freeReg = scope.firstTemp;
        fn.firstTemp = scope.firstTemp;
    }

    // Compiles the function into its own prototype. The enclosing function stays the current one once it returns
    std::unique_ptr<Prototype> Compiler::CompileFunction(FuncDecl& _node)
    {
        auto proto = std::make_unique<Prototype>();
        proto->name = _node.name;

        if (_node.params.size() >= MaxRegisters)
            Report(_node, "too many parameters in function '" + _node.name + "'");
        proto->numParams = static_cast<uint8_t>(_node.params.size());

        m_funcs.emplace_back();
        FuncState& fn = Fn();
        fn.proto = proto.get();

        // The parameters are the first locals, one register each
        for (Param const& param : _node.params)
        {
            uint8_t reg = AllocReg(_node);
            fn.locals.push_back({ param.name, reg });
        }
        fn.firstTemp = fn.freeReg;

        // The body shares the scope of the parameters
        for (NodePtr& statement : _node.body->statements)
            statement->Accept(*this);

        // A function that reaches its end returns nothing
        uint32_t lastRow = proto->rows.empty() ? _node.row : proto->rows.back();
        proto->Emit(EncodeABC(OpCode::Return, 0, 0), lastRow);

        m_funcs.pop_back();
        return proto;
    }

    void Compiler::Visit(FuncDecl& _node)
    {
        bool global = IsGlobalScope();
        FuncState& fn = Fn();
        size_t saved = fn.freeReg;

        uint8_t reg = AllocReg(_node);

        // A local function is visible in its own body, so recursion captures it as an upvalue
        if (global == false)
        {
            fn.locals.push_back({ _node.name, reg });
            fn.firstTemp = fn.freeReg;
        }

        std::unique_ptr<Prototype> proto = CompileFunction(_node);

        int32_t index = Proto().AddProto(std::move(proto));
        if (index < 0)
        {
            Report(_node, "too many functions in one function");
            index = 0;
        }
        Emit(EncodeABx(OpCode::Closure, reg, static_cast<uint16_t>(index)), _node);

        if (global)
        {
            uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.name)), _node);
            Emit(EncodeABx(OpCode::SetGlobal, reg, k), _node);
            fn.freeReg = saved;
        }
    }
}
