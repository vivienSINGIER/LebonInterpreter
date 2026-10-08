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
            if ((_l).IsNumber() == false || (_r).IsNumber() == false) { SAVE_FRAME(); return RuntimeError("operands must be numbers"); }
        #define CHECK_STRINGS(_l, _r) \
            if ((_l).IsString() == false || (_r).IsString() == false) { SAVE_FRAME(); return RuntimeError("operands must be strings"); }
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
        m_frames.resize(MaxFrames);
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

        m_openUpvalues = nullptr;
        m_globals.assign(_program.GlobalCount(), Value());
        DefineNative(_program, "afise", 1, &Afise);

        FunctionObj* main = m_heap.New<FunctionObj>(_program.main.get());
        m_stack[0] = Value::MakeObj(ValueType::Function, main);

        m_executed = 0;
        m_opCounts.fill(0);

        // La frame de main est la première du tableau, ses registres commencent juste après la case de la fonction.
        // Pas de test de dépassement ici : main n'a pas plus de MaxRegisters registres, la pile est bien plus grande
        CallFrame& first = m_frames[0];
        first.closure = main;
        first.ip = main->proto->code.data();
        first.base = &m_stack[1];
        first.constants = main->proto->constants.data();
        m_top = &first;

        bool ok = m_statsEnabled ? Execute<true>() : Execute<false>();
        if (ok == false)
            CloseUpvalues(m_stack.data());

        m_top = nullptr;
        return ok;
    }

    // Logge une erreur d'exécution avec la fonction et la ligne source de l'instruction en cours, renvoie toujours false.
    // m_top et l'ip de la frame courante doivent être à jour (SAVE_FRAME)
    bool VM::RuntimeError(std::string const& _message)
    {
        std::string text = _message;

        if (m_top != nullptr)
        {
            CallFrame const& frame = *m_top;
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
    // et rechargés quand on change de frame (appel ou retour). Les frames sont dans un tableau fixe : appeler
    // ou revenir déplace seulement le pointeur "frame", sans allocation ni test de capacité
    template <bool CountInstructions>
    bool VM::Execute()
    {
        CallFrame* const frameBase = m_frames.data();
        CallFrame* const frameEnd = frameBase + MaxFrames;
        Value* const stackEnd = m_stack.data() + m_stack.size();

        CallFrame* frame = m_top;
        Instruction const* ip = nullptr;
        Value* base = nullptr;
        Value const* k = nullptr;

#define LOAD_FRAME() \
        ip = frame->ip; \
        base = frame->base; \
        k = frame->constants

        // Avant de signaler une erreur : la frame courante et son ip doivent être lisibles par RuntimeError
#define SAVE_FRAME() \
        frame->ip = ip; \
        m_top = frame

        LOAD_FRAME();

        for (;;)
        {
            Instruction i = *ip++;

            if constexpr (CountInstructions)
            {
                m_executed++;
                m_opCounts[static_cast<size_t>(GetOp(i)) % m_opCounts.size()]++;
            }

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

            // Opérations dont le second opérande est une constante du pool de la fonction : k[C]
            case OpCode::AddK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n + right.n);
                break;
            }

            case OpCode::ConcatK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_STRINGS(left, right)
                base[GetA(i)] = Value::MakeString(m_heap.Intern(left.AsString()->chars + right.AsString()->chars));
                break;
            }

            case OpCode::SubK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n - right.n);
                break;
            }

            case OpCode::MulK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n * right.n);
                break;
            }

            case OpCode::DivK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeNumber(left.n / right.n);
                break;
            }

            case OpCode::EqK:
                base[GetA(i)] = MakeBool(base[GetB(i)] == k[GetC(i)]);
                break;

            case OpCode::LtK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n < right.n);
                break;
            }

            case OpCode::LeK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n <= right.n);
                break;
            }

            case OpCode::GtK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
                CHECK_NUMBERS(left, right)
                base[GetA(i)] = MakeBool(left.n > right.n);
                break;
            }

            case OpCode::GeK:
            {
                Value const& left = base[GetB(i)];
                Value const& right = k[GetC(i)];
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

                if (slot->type == ValueType::Function)
                {
                    // Chemin des fonctions du programme : tout ce qu'il faut sur la fonction est lu une seule fois
                    FunctionObj* function = static_cast<FunctionObj*>(slot->o);
                    Prototype const* proto = function->proto;

                    // Les arguments sont déjà aux bons registres : la nouvelle frame commence juste après la fonction
                    Value* newBase = slot + 1;
                    CallFrame* next = frame + 1;

                    if (argCount != proto->numParams)
                    {
                        SAVE_FRAME();
                        return RuntimeError("function expects " + std::to_string(proto->numParams) + " argument(s), got " + std::to_string(argCount));
                    }

                    if (next == frameEnd || newBase + proto->maxRegisters > stackEnd)
                    {
                        SAVE_FRAME();
                        return RuntimeError("stack overflow, too many nested calls");
                    }

                    frame->ip = ip;     // où l'appelant reprendra

                    next->closure = function;
                    next->base = newBase;
                    next->constants = proto->constants.data();
                    next->ip = proto->code.data();

                    frame = next;
                    LOAD_FRAME();
                }
                else if (slot->type == ValueType::Native)
                {
                    NativeObj* native = static_cast<NativeObj*>(slot->o);
                    if (argCount != native->arity)
                    {
                        SAVE_FRAME();
                        return RuntimeError("'" + native->name + "' expects " + std::to_string(native->arity) + " argument(s), got " + std::to_string(argCount));
                    }

                    *slot = native->fn(*this, slot + 1, argCount);
                }
                else
                {
                    SAVE_FRAME();
                    return RuntimeError(std::string("attempt to call a ") + TypeName(slot->type));
                }
                break;
            }

            case OpCode::Return:
            {
                // Le résultat prend la place de la fonction appelée, juste avant le premier registre de la frame
                if (GetB(i) != 0)
                    base[-1] = base[GetA(i)];
                else
                    base[-1] = Value();

                // Les variables capturées par des fermetures survivent à la frame : on les copie hors de la pile.
                // Presque toujours il n'y en a aucune d'ouverte dans cette frame, on évite alors l'appel
                if (m_openUpvalues != nullptr && m_openUpvalues->location >= base)
                    CloseUpvalues(base);

                if (frame == frameBase)
                    return true;

                frame--;
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
                SAVE_FRAME();
                return RuntimeError("invalid instruction");
            }
        }

#undef SAVE_FRAME
#undef LOAD_FRAME
    }
}
