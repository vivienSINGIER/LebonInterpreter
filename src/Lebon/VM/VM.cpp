#include "VM.h"

#include "../core/Error.h"

namespace Bytecode
{
    namespace
    {
        // Le compilateur garantit les types (l'analyseur les a vérifiés), on ne les teste donc qu'en debug
        // pour attraper un bytecode écrit à la main ou un bug du compilateur
#ifndef NDEBUG
        #define CHECK_NUMBERS(_l, _r) \
            if ((_l).IsNumber() == false || (_r).IsNumber() == false) { frame->ip = ip; return RuntimeError("operands must be numbers"); }
        #define CHECK_STRINGS(_l, _r) \
            if ((_l).IsString() == false || (_r).IsString() == false) { frame->ip = ip; return RuntimeError("operands must be strings"); }
#else
        #define CHECK_NUMBERS(_l, _r)
        #define CHECK_STRINGS(_l, _r)
#endif

        Value MakeNumber(float _n)
        {
            Value value;
            value.type = ValueType::Number;
            value.n = _n;
            return value;
        }

        Value MakeBool(bool _b)
        {
            Value value;
            value.type = ValueType::Bool;
            value.b = _b;
            return value;
        }

        char const* TypeName(ValueType _type)
        {
            switch (_type)
            {
            case ValueType::Nil:      return "nil";
            case ValueType::Bool:     return "bool";
            case ValueType::Number:   return "number";
            case ValueType::String:   return "string";
            case ValueType::Function: return "function";
            case ValueType::Native:   return "native function";
            }
            return "unknown";
        }

        // afise : écrit la valeur suivie d'un retour à la ligne, ne renvoie rien
        Value Afise(VM& _vm, Value const* _args, int)
        {
            _vm.Out() << ToString(_args[0]) << "\n";
            return Value();
        }
    }

    VM::VM(Heap& _heap)
        : m_heap(_heap)
        , m_stack(StackSize)
    {
        m_frames.reserve(MaxFrames);
    }

    // Installe un natif dans la case de la globale du même nom, ignoré si le programme ne la connaît pas
    void VM::DefineNative(CompiledProgram const& _program, std::string const& _name, int _arity, NativeFn _fn)
    {
        size_t slot = _program.FindGlobal(_name);
        if (slot == CompiledProgram::NoGlobal)
            return;

        NativeObj* native = m_heap.New<NativeObj>(_name, _arity, _fn);
        m_globals[slot] = Value::MakeObj(ValueType::Native, native);
    }

    // Prépare une exécution neuve : globales à nil + natifs, puis lance main comme une fonction sans argument
    bool VM::Run(CompiledProgram const& _program)
    {
        if (_program.main == nullptr)
        {
            ErrorManager::LogError(Error::Execution("nothing to run, the compilation failed", 0, 0));
            return false;
        }

        m_frames.clear();
        m_openUpvalues = nullptr;
        m_globals.assign(_program.GlobalCount(), Value());
        DefineNative(_program, "afise", 1, &Afise);

        FunctionObj* main = m_heap.New<FunctionObj>(_program.main.get());
        m_stack[0] = Value::MakeObj(ValueType::Function, main);

        bool ok = PushFrame(main, &m_stack[1]) && Execute();
        if (ok == false)
        {
            CloseUpvalues(m_stack.data());
            m_frames.clear();
        }
        return ok;
    }

    // Empile une frame : refuse si la profondeur d'appel ou la pile dépasse sa limite
    bool VM::PushFrame(FunctionObj* _closure, Value* _base)
    {
        Value* stackEnd = m_stack.data() + m_stack.size();

        if (m_frames.size() >= MaxFrames || _base + _closure->proto->maxRegisters > stackEnd)
            return RuntimeError("stack overflow, too many nested calls");

        CallFrame frame;
        frame.closure = _closure;
        frame.ip = _closure->proto->code.data();
        frame.base = _base;
        m_frames.push_back(frame);
        return true;
    }

    // Logge une erreur d'exécution avec la fonction et la ligne source de l'instruction en cours, renvoie toujours false.
    // L'ip de la frame courante doit être à jour
    bool VM::RuntimeError(std::string const& _message)
    {
        std::string text = _message;

        if (m_frames.empty() == false)
        {
            CallFrame const& frame = m_frames.back();
            Prototype const& proto = *frame.closure->proto;

            size_t index = static_cast<size_t>(frame.ip - proto.code.data());
            if (index > 0)
                index--;    // ip pointe sur l'instruction suivante

            text += " (in " + (proto.name.empty() ? std::string("<main>") : "'" + proto.name + "'");
            if (index < proto.rows.size())
                text += ", line " + std::to_string(proto.rows[index]);
            text += ")";
        }

        ErrorManager::LogError(Error::Execution(text, 0, 0));
        return false;
    }

    // Renvoie l'upvalue qui désigne cette case de la pile : celle qui existe déjà si une autre fermeture l'a capturée,
    // sinon une nouvelle, insérée dans la liste des ouvertes
    UpvalueObj* VM::CaptureUpvalue(Value* _local)
    {
        UpvalueObj* previous = nullptr;
        UpvalueObj* current = m_openUpvalues;

        while (current != nullptr && current->location > _local)
        {
            previous = current;
            current = current->nextOpen;
        }

        if (current != nullptr && current->location == _local)
            return current;

        UpvalueObj* created = m_heap.New<UpvalueObj>(_local);
        created->nextOpen = current;

        if (previous != nullptr)
            previous->nextOpen = created;
        else
            m_openUpvalues = created;

        return created;
    }

    // Ferme toutes les upvalues ouvertes à partir de cette case : la valeur est copiée dans l'upvalue,
    // qui ne dépend alors plus de la pile (appelé quand une fonction se termine)
    void VM::CloseUpvalues(Value* _last)
    {
        while (m_openUpvalues != nullptr && m_openUpvalues->location >= _last)
        {
            UpvalueObj* upvalue = m_openUpvalues;
            upvalue->closed = *upvalue->location;
            upvalue->location = &upvalue->closed;

            m_openUpvalues = upvalue->nextOpen;
            upvalue->nextOpen = nullptr;
        }
    }

    // Boucle principale : lit une instruction, l'exécute, recommence jusqu'au RETURN de main.
    // ip, base et les constantes de la fonction courante sont gardés dans des variables locales,
    // et rechargés quand on change de frame (appel ou retour)
    bool VM::Execute()
    {
        CallFrame* frame = nullptr;
        Instruction const* ip = nullptr;
        Value* base = nullptr;
        Value const* k = nullptr;

#define LOAD_FRAME() \
        frame = &m_frames.back(); \
        ip = frame->ip; \
        base = frame->base; \
        k = frame->closure->proto->constants.data()

        LOAD_FRAME();

        for (;;)
        {
            Instruction i = *ip++;

            switch (GetOp(i))
            {
            case OpCode::Move:
                base[GetA(i)] = base[GetB(i)];
                break;

            case OpCode::LoadK:
                base[GetA(i)] = k[GetBx(i)];
                break;

            case OpCode::LoadBool:
                base[GetA(i)] = MakeBool(GetB(i) != 0);
                break;

            case OpCode::LoadNil:
                base[GetA(i)] = Value();
                break;

            case OpCode::GetGlobal:
                base[GetA(i)] = m_globals[GetBx(i)];
                break;

            case OpCode::SetGlobal:
                m_globals[GetBx(i)] = base[GetA(i)];
                break;

            case OpCode::GetUpval:
                base[GetA(i)] = *frame->closure->upvalues[GetB(i)]->location;
                break;

            case OpCode::SetUpval:
                *frame->closure->upvalues[GetB(i)]->location = base[GetA(i)];
                break;

            case OpCode::Add:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n + right.n);
                break;
            }

            case OpCode::Concat:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_STRINGS(left, right)
                // Le résultat est interné pour que l'égalité de deux chaînes reste une comparaison de pointeurs
                base[GetA(i)] = Value::MakeString(m_heap.Intern(left.AsString()->chars + right.AsString()->chars));
                break;
            }

            case OpCode::Sub:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n - right.n);
                break;
            }

            case OpCode::Mul:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n * right.n);
                break;
            }

            case OpCode::Div:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n / right.n);    // diviser par zéro donne inf ou nan, comme un float
                break;
            }

            case OpCode::Neg:
            {
                Value const& operand = base[GetB(i)];
                CHECK_NUMBERS(operand, operand);
                base[GetA(i)] = MakeNumber(-operand.n);
                break;
            }

            case OpCode::Not:
                base[GetA(i)] = MakeBool(base[GetB(i)].IsTruthy() == false);
                break;

            case OpCode::Eq:
                base[GetA(i)] = MakeBool(base[GetB(i)] == base[GetC(i)]);
                break;

            case OpCode::Lt:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n < right.n);
                break;
            }

            case OpCode::Le:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n <= right.n);
                break;
            }

            case OpCode::Gt:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n > right.n);
                break;
            }

            case OpCode::Ge:
            {
                Value const& left = base[GetB(i)];
                Value const& right = base[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n >= right.n);
                break;
            }

            case OpCode::Jmp:
                ip += GetSBx(i);
                break;

            case OpCode::JmpIfNot:
                if (base[GetA(i)].IsTruthy() == false)
                    ip += GetSBx(i);
                break;

            case OpCode::Call:
            {
                // La fonction est dans R[A], ses arguments dans R[A+1] .. R[A+B]. Le résultat remplacera la fonction
                Value* slot = base + GetA(i);
                int argCount = GetB(i);
                Value callee = *slot;

                frame->ip = ip;

                if (callee.type == ValueType::Function)
                {
                    FunctionObj* function = static_cast<FunctionObj*>(callee.o);
                    if (argCount != function->proto->numParams)
                        return RuntimeError("function expects " + std::to_string(function->proto->numParams) + " argument(s), got " + std::to_string(argCount));

                    // Les arguments sont déjà aux bons registres : la nouvelle frame commence juste après la fonction
                    if (PushFrame(function, slot + 1) == false)
                        return false;
                    LOAD_FRAME();
                }
                else if (callee.type == ValueType::Native)
                {
                    NativeObj* native = static_cast<NativeObj*>(callee.o);
                    if (argCount != native->arity)
                        return RuntimeError("'" + native->name + "' expects " + std::to_string(native->arity) + " argument(s), got " + std::to_string(argCount));

                    *slot = native->fn(*this, slot + 1, argCount);
                }
                else
                {
                    return RuntimeError(std::string("attempt to call a ") + TypeName(callee.type));
                }
                break;
            }

            case OpCode::Return:
            {
                Value result = GetB(i) != 0 ? base[GetA(i)] : Value();

                // Les variables capturées par des fermetures survivent à la frame : on les copie hors de la pile
                CloseUpvalues(base);

                Value* resultSlot = base - 1;
                m_frames.pop_back();
                *resultSlot = result;

                if (m_frames.empty())
                    return true;

                LOAD_FRAME();
                break;
            }

            case OpCode::Closure:
            {
                Prototype const* proto = frame->closure->proto->protos[GetBx(i)].get();
                FunctionObj* function = m_heap.New<FunctionObj>(proto);

                // Chaque variable capturée vient d'un registre de la fonction courante (parent) ou de ses propres upvalues
                function->upvalues.reserve(proto->upvalues.size());
                for (UpvalDesc const& desc : proto->upvalues)
                {
                    if (desc.fromParentRegister)
                        function->upvalues.push_back(CaptureUpvalue(base + desc.index));
                    else
                        function->upvalues.push_back(frame->closure->upvalues[desc.index]);
                }

                base[GetA(i)] = Value::MakeObj(ValueType::Function, function);
                break;
            }

            case OpCode::Count:
            default:
                frame->ip = ip;
                return RuntimeError("invalid instruction");
            }
        }

#undef LOAD_FRAME
    }
}
