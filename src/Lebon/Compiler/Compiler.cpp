#include "Compiler.h"

#include <charconv>

#include "../core/Error.h"

namespace Bytecode
{
    // Point d'entrée : repart d'un main vide et y compile tout le programme.
    // Le résultat porte aussi les noms des globales, dans l'ordre des slots utilisés par GETGLOBAL / SETGLOBAL
    CompiledProgram Compiler::Compile(Program& _program)
    {
        CompiledProgram result;
        result.main = std::make_unique<Prototype>();

        m_funcs.clear();
        m_funcs.emplace_back();
        Func().proto = result.main.get();

        m_symbols = &_program.stack.table;
        m_target = 0;
        m_errorCount = 0;
        m_registersReported = false;

        _program.Accept(*this);

        result.globalNames = m_symbols->GlobalNames();

        m_funcs.clear();
        m_symbols = nullptr;
        if (m_errorCount > 0)
            return CompiledProgram();

        return result;
    }

    // Vrai dans main hors de tout bloc : une variable déclarée ici est une globale
    bool Compiler::IsGlobalScope()
    {
        return m_funcs.size() == 1 && Func().scopes.empty();
    }

    // Compile une expression pour que sa valeur finisse dans le registre _dst
    void Compiler::CompileTo(Node& _node, uint8_t _dst)
    {
        m_target = _dst;
        _node.Accept(*this);
    }

    // Cherche une variable locale de la fonction, de la plus récente à la plus ancienne (le shadowing marche)
    LocalVar* Compiler::FindLocal(FuncState& _fn, std::string const& _name)
    {
        for (size_t i = _fn.locals.size(); i > 0; --i)
        {
            if (_fn.locals[i - 1].name == _name)
                return &_fn.locals[i - 1];
        }
        return nullptr;
    }

    // Cherche le nom dans les fonctions parentes. Local du parent : capturé depuis son registre.
    // Plus haut : capturé via l'upvalue du parent. Un nom n'est capturé qu'une fois par fonction.
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

    // Réserve le prochain registre libre et agrandit la taille de frame de la fonction si besoin
    uint8_t Compiler::AllocReg(Node const& _at)
    {
        FuncState& fn = Func();
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

    // Vrai si c'est le dernier temporaire alloué : on peut le réutiliser tel quel
    bool Compiler::IsTopTemp(uint8_t _register)
    {
        FuncState const& func = Func();
        return _register >= func.firstTemp && static_cast<size_t>(_register) + 1 == func.freeReg;
    }

    // Ajoute une instruction à la fonction courante avec la ligne source du noeud
    size_t Compiler::Emit(Instruction _i, Node const& _at)
    {
        return Proto().Emit(_i, _at.row);
    }

    // Index de la constante dans le pool de la fonction (ajoutée si nouvelle), erreur si le pool est plein
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

    // Slot de la globale dans le tableau de la VM : les globales sont lues par index, plus par nom
    uint16_t Compiler::GlobalSlot(Semantics::SymbolId _id, Node const& _at)
    {
        uint32_t slot = Semantics::InvalidGlobalSlot;
        if (_id != Semantics::InvalidSymbolId)
            slot = m_symbols->Get(_id).globalSlot;

        if (slot == Semantics::InvalidGlobalSlot)
        {
            Report(_at, "name isn't resolved to a global");
            return 0;
        }
        if (slot > MaxBx)
        {
            Report(_at, "too many globals in the program");
            return 0;
        }
        return static_cast<uint16_t>(slot);
    }

    // Journalise une erreur à la position du noeud, Compile renverra alors nullptr
    void Compiler::Report(Node const& _at, std::string const& _message)
    {
        ErrorManager::LogError(Error::Execution(_message, _at.row, _at.column));
        m_errorCount++;
    }

    // Expressions
    // Relit le texte du nombre en float puis le charge depuis le pool de constantes
    void Compiler::Visit(NumberLiteral& _node)
    {
        float number = _node.value;

        uint16_t k = ConstantIndex(Value::MakeNumber(number), _node);
        Emit(EncodeABx(OpCode::LoadK, m_target, k), _node);
    }

    // La chaîne est internée dans le heap puis chargée comme constante
    void Compiler::Visit(StringLiteral& _node)
    {
        uint16_t k = ConstantIndex(Value::MakeString(m_heap.Intern(_node.value)), _node);
        Emit(EncodeABx(OpCode::LoadK, m_target, k), _node);
    }

    // Un booléen tient dans l'instruction, pas besoin de constante
    void Compiler::Visit(BooleanLiteral& _node)
    {
        Emit(EncodeABC(OpCode::LoadBool, m_target, _node.value ? 1 : 0), _node);
    }

    // Lecture d'une variable : locale (MOVE), sinon upvalue (GETUPVAL), sinon globale (GETGLOBAL)
    void Compiler::Visit(Identifier& _node)
    {
        if (LocalVar* local = FindLocal(Func(), _node.name))
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

        Emit(EncodeABx(OpCode::GetGlobal, m_target, GlobalSlot(_node.symbol, _node)), _node);
    }

    // Moins unaire : compile l'opérande (dans _dst si c'est un temporaire libre) puis NEG
    void Compiler::Visit(UnaryExpr& _node)
    {
        if (_node.op != TokenType::SUB)
        {
            Report(_node, "unsupported unary operator");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = Func().freeReg;

        uint8_t operand = dst;
        if (IsTopTemp(dst) == false)
            operand = AllocReg(_node);

        CompileTo(*_node.operand, operand);
        Emit(EncodeABC(OpCode::Neg, dst, operand), _node);

        Func().freeReg = saved;
    }

    // Opération binaire : choisit l'opcode, compile le côté gauche puis le droit dans deux registres, puis émet l'opération.
    // L'addition est typée par l'analyseur : ADD pour les nombres, CONCAT pour les chaînes, la VM n'a rien à tester.
    // Les comparaisons laissent un bool dans le registre cible. != est un EQ suivi d'un NOT
    void Compiler::Visit(BinaryExpr& _node)
    {
        OpCode op = OpCode::Add;
        bool negate = false;
        switch (_node.op)
        {
        case TokenType::EQ:
            op = OpCode::Eq; break;
        case TokenType::NEQ:
            op = OpCode::Eq; negate = true; break;
        case TokenType::LT:
            op = OpCode::Lt; break;
        case TokenType::GT:
            op = OpCode::Gt; break;
        case TokenType::LE:
            op = OpCode::Le; break;
        case TokenType::GE:
            op = OpCode::Ge; break;
        case TokenType::ADD:
            if (_node.type == Semantics::InferredType::Number)
                op = OpCode::Add;
            else if (_node.type == Semantics::InferredType::String)
                op = OpCode::Concat;
            else
            {
                Report(_node, "can't infer the type of this addition");
                return;
            }
            break;
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
        size_t saved = Func().freeReg;

        uint8_t left = dst;
        if (IsTopTemp(dst) == false)
            left = AllocReg(_node);
        CompileTo(*_node.left, left);

        uint8_t right = AllocReg(_node);
        CompileTo(*_node.right, right);

        Emit(EncodeABC(op, dst, left, right), _node);
        if (negate)
            Emit(EncodeABC(OpCode::Not, dst, dst), _node);

        Func().freeReg = saved;
    }

    // Appel : la fonction dans un registre "base", les arguments dans les registres qui suivent, puis CALL.
    // Le résultat remplace la fonction en base, il est recopié dans _dst si besoin
    void Compiler::Visit(CallExpr& _node)
    {
        if (_node.args.size() >= MaxRegisters)
        {
            Report(_node, "too many arguments in a call");
            return;
        }

        uint8_t dst = m_target;
        size_t saved = Func().freeReg;

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

        Func().freeReg = saved;
    }

    // Affectation utilisée comme expression : sa valeur est aussi laissée dans _dst
    void Compiler::Visit(AssignExpr& _node)
    {
        CompileAssign(_node, true);
    }

    // Range la valeur dans la variable. _wantValue : le résultat doit aussi finir dans m_target.
    // Local : valeur construite directement dans son registre. Sinon : valeur dans un registre puis SETUPVAL ou SETGLOBAL
    void Compiler::CompileAssign(AssignExpr& _node, bool _wantValue)
    {
        uint8_t dst = m_target;
        size_t saved = Func().freeReg;

        if (LocalVar* local = FindLocal(Func(), _node.name))
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
            Emit(EncodeABx(OpCode::SetGlobal, valueReg, GlobalSlot(_node.symbol, _node)), _node);
        }

        Func().freeReg = saved;
    }

    // Statements
    // Expression utilisée comme instruction : sa valeur est jetée, les registres sont libérés ensuite
    void Compiler::Visit(ExprStmt& _node)
    {
        size_t saved = Func().freeReg;

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

        Func().freeReg = saved;
    }

    // Compile chaque instruction du programme puis termine main par un RETURN
    void Compiler::Visit(Program& _node)
    {
        for (NodePtr& statement : _node.statements)
            statement->Accept(*this);

        uint32_t lastRow = Proto().rows.empty() ? 0 : Proto().rows.back();
        Proto().Emit(EncodeABC(OpCode::Return, 0, 0), lastRow);
    }

    // Variables
    // Déclaration : la valeur initiale (ou nil) est calculée dans un registre.
    // Global : SETGLOBAL puis le registre est libéré. Local : le registre devient la variable
    void Compiler::Visit(VarDecl& _node)
    {
        FuncState& fn = Func();
        size_t saved = fn.freeReg;

        uint8_t reg = AllocReg(_node);

        if (_node.init)
            CompileTo(*_node.init, reg);
        else
            Emit(EncodeABC(OpCode::LoadNil, reg), _node);

        if (IsGlobalScope())
        {
            Emit(EncodeABx(OpCode::SetGlobal, reg, GlobalSlot(_node.symbol, _node)), _node);
            fn.freeReg = saved;
        }
        else
        {
            fn.locals.push_back({ _node.name, reg });
            fn.firstTemp = fn.freeReg;
        }
    }

    // Sans valeur : RETURN vide. Avec valeur : calculée dans un temporaire puis renvoyée
    void Compiler::Visit(ReturnStmt& _node)
    {
        if (_node.value == nullptr)
        {
            Emit(EncodeABC(OpCode::Return, 0, 0), _node);
            return;
        }

        size_t saved = Func().freeReg;

        uint8_t reg = AllocReg(_node);
        CompileTo(*_node.value, reg);
        Emit(EncodeABC(OpCode::Return, reg, 1), _node);

        Func().freeReg = saved;
    }

    // Fait pointer le saut déjà émis vers la prochaine instruction. Erreur si la distance ne tient pas dans l'instruction
    void Compiler::PatchJumpHere(size_t _jump, Node const& _at)
    {
        size_t distance = Proto().Here() - _jump - 1;
        if (distance > static_cast<size_t>(MaxSBx))
        {
            Report(_at, "this block is too long to be jumped over");
            return;
        }

        Proto().PatchJump(_jump, Proto().Here());
    }

    // Condition : le test est calculé dans un temporaire libéré aussitôt. JMPIFNOT saute le bloc "alors" si le test est faux.
    // Avec un sinon, le bloc "alors" se termine par un JMP qui saute le bloc "sinon", et JMPIFNOT arrive au début de celui-ci
    void Compiler::Visit(IfStmt& _node)
    {
        FuncState& fn = Func();
        size_t saved = fn.freeReg;

        uint8_t test = AllocReg(_node);
        CompileTo(*_node.condition, test);
        size_t skipThen = Emit(EncodeAsBx(OpCode::JmpIfNot, test, 0), *_node.condition);
        fn.freeReg = saved;

        _node.thenBranch->Accept(*this);

        if (_node.elseBranch)
        {
            size_t skipElse = Emit(EncodeAsBx(OpCode::Jmp, 0, 0), _node);
            PatchJumpHere(skipThen, _node);

            _node.elseBranch->Accept(*this);
            PatchJumpHere(skipElse, _node);
        }
        else
        {
            PatchJumpHere(skipThen, _node);
        }
    }

    // Ouvre une portée, compile les instructions, puis oublie les locales du bloc et libère leurs registres
    void Compiler::Visit(Block& _node)
    {
        FuncState& fn = Func();
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

    // Compile la fonction dans son propre prototype : paramètres en premiers registres, corps, RETURN final.
    // La fonction englobante redevient la fonction courante au retour
    std::unique_ptr<Prototype> Compiler::CompileFunction(FuncDecl& _node)
    {
        auto proto = std::make_unique<Prototype>();
        proto->name = _node.name;

        if (_node.params.size() >= MaxRegisters)
            Report(_node, "too many parameters in function '" + _node.name + "'");
        proto->numParams = static_cast<uint8_t>(_node.params.size());

        m_funcs.emplace_back();
        FuncState& fn = Func();
        fn.proto = proto.get();

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

    // Déclaration de fonction : compile le corps dans un prototype, CLOSURE crée la fonction dans un registre,
    // puis SETGLOBAL si on est au niveau global (sinon le registre est la variable locale)
    void Compiler::Visit(FuncDecl& _node)
    {
        bool global = IsGlobalScope();
        FuncState& fn = Func();
        size_t saved = fn.freeReg;

        uint8_t reg = AllocReg(_node);

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
            Emit(EncodeABx(OpCode::SetGlobal, reg, GlobalSlot(_node.symbol, _node)), _node);
            fn.freeReg = saved;
        }
    }
}
