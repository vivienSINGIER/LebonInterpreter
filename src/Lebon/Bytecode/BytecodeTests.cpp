#include "BytecodeTests.h"

#include <iostream>
#include <memory>
#include <sstream>

#include "Disassembler.h"
#include "Heap.hpp"
#include "Prototype.hpp"
#include "../core/Error.h"
#include "../VM/VM.h"

namespace Bytecode
{
    namespace
    {
        uint32_t g_failures = 0;

        void Check(bool _ok, char const* _what)
        {
            if (_ok)
                return;
            std::cerr << "[bytecode] FAIL " << _what << "\n";
            g_failures++;
        }

        void TestEncoding()
        {
            Instruction abc = EncodeABC(OpCode::Add, 7, 200, 255);
            Check(GetOp(abc) == OpCode::Add && GetA(abc) == 7 && GetB(abc) == 200 && GetC(abc) == 255, "ABC round trip");

            Instruction abx = EncodeABx(OpCode::LoadK, 3, 65535);
            Check(GetOp(abx) == OpCode::LoadK && GetA(abx) == 3 && GetBx(abx) == 65535, "ABx round trip");

            Instruction back = EncodeAsBx(OpCode::Jmp, 0, -5);
            Instruction forth = EncodeAsBx(OpCode::JmpIfNot, 9, 1234);
            Check(GetSBx(back) == -5, "sBx negative");
            Check(GetSBx(forth) == 1234 && GetA(forth) == 9, "sBx positive");
            Check(GetSBx(EncodeAsBx(OpCode::Jmp, 0, -MaxSBx)) == -MaxSBx, "sBx lower bound");
            Check(GetSBx(EncodeAsBx(OpCode::Jmp, 0, MaxSBx)) == MaxSBx, "sBx upper bound");
        }

        void TestValues()
        {
            Heap heap;
            Check(heap.Intern("lebon") == heap.Intern("lebon"), "strings are interned");
            Check(heap.Intern("lebon") != heap.Intern("Lebon"), "different strings stay apart");

            Value nil;
            Check(nil.IsNil() && !nil.IsTruthy(), "default value is nil");
            Check(!Value::MakeBool(false).IsTruthy() && Value::MakeBool(true).IsTruthy(), "bool truthiness");
            Check(Value::MakeNumber(0).IsTruthy(), "0 is true");
            Check(Value::MakeNumber(2) == Value::MakeNumber(2) && Value::MakeNumber(2) != Value::MakeNumber(3), "number equality");
            Check(Value::MakeString(heap.Intern("a")) == Value::MakeString(heap.Intern("a")), "string equality");
            Check(Value::MakeNumber(1) != Value::MakeBool(true), "types are never equal");
        }

        void TestConstantPool()
        {
            Heap heap;
            Prototype p;
            int32_t two = p.AddConstant(Value::MakeNumber(2));
            int32_t text = p.AddConstant(Value::MakeString(heap.Intern("salu")));
            Check(p.AddConstant(Value::MakeNumber(2)) == two, "number constant is reused");
            Check(p.AddConstant(Value::MakeString(heap.Intern("salu"))) == text, "string constant is reused");
            Check(p.AddConstant(Value::MakeNumber(0.0)) != p.AddConstant(Value::MakeNumber(-0.0)), "0 and -0 stay apart");
            Check(p.AddConstant(Value::MakeBool(true)) != p.AddConstant(Value::MakeBool(false)), "bools stay apart");
            Check(p.AddConstant(Value::MakeNil()) == p.AddConstant(Value::MakeNil()), "nil is reused");
        }

        void TestJumpPatching()
        {
            Prototype p;
            size_t jump = p.Emit(EncodeAsBx(OpCode::JmpIfNot, 1, 0), 1);   // target unknown yet
            p.Emit(EncodeABC(OpCode::LoadNil, 2), 2);
            p.Emit(EncodeABC(OpCode::LoadNil, 3), 2);
            p.PatchJump(jump, p.Here());                                   // jumps over the 2 LOADNIL
            Check(GetSBx(p.code[jump]) == 2 && GetA(p.code[jump]) == 1, "forward jump patched");

            size_t loop = p.Emit(EncodeABC(OpCode::Move, 0, 1), 3);
            size_t back = p.Emit(EncodeAsBx(OpCode::Jmp, 0, 0), 3);
            p.PatchJump(back, loop);                                       // jumps back to the MOVE
            Check(GetSBx(p.code[back]) == -2, "backward jump patched");
        }

        void TestCompiledProgram()
        {
            CompiledProgram empty;
            Check(!empty && empty.GlobalCount() == 0, "empty result is a failed compilation");

            CompiledProgram program;
            program.main = std::make_unique<Prototype>();
            program.globalNames = { "afise", "total", "x" };
            Check(static_cast<bool>(program) && program.GlobalCount() == 3, "result with a main is a success");
            Check(program.FindGlobal("afise") == 0 && program.FindGlobal("x") == 2, "global slot found by name");
            Check(program.FindGlobal("inconnu") == CompiledProgram::NoGlobal, "unknown global has no slot");
        }

        // Lance un programme écrit à la main, renvoie ce qu'afise a écrit dans _output.
        // Les erreurs d'exécution sont loggées sur cerr : on les masque, ces tests en provoquent exprès
        bool RunProgram(CompiledProgram const& _program, Heap& _heap, std::string& _output)
        {
            std::ostringstream out;
            std::ostringstream errors;
            std::streambuf* previous = std::cerr.rdbuf(errors.rdbuf());

            VM vm(_heap);
            vm.SetOutput(out);
            bool ok = vm.Run(_program);

            std::cerr.rdbuf(previous);
            ErrorManager::Clear();

            _output = out.str();
            return ok;
        }

        // Une boucle écrite à la main : le compilateur ne produit ni comparaison ni saut pour l'instant,
        // ce test est le seul à exécuter Gt, JmpIfNot et Jmp.   somme = 0 ; i = 3 ; tant que i > 0 : somme += i ; i -= 1
        void TestVmLoop()
        {
            Heap heap;
            CompiledProgram program;
            program.globalNames = { "afise" };
            program.main = std::make_unique<Prototype>();

            Prototype& main = *program.main;
            main.maxRegisters = 7;
            uint16_t zero = static_cast<uint16_t>(main.AddConstant(Value::MakeNumber(0)));
            uint16_t three = static_cast<uint16_t>(main.AddConstant(Value::MakeNumber(3)));
            uint16_t one = static_cast<uint16_t>(main.AddConstant(Value::MakeNumber(1)));

            main.Emit(EncodeABx(OpCode::LoadK, 0, zero), 1);       // somme
            main.Emit(EncodeABx(OpCode::LoadK, 1, three), 1);      // i
            main.Emit(EncodeABx(OpCode::LoadK, 2, zero), 1);
            main.Emit(EncodeABx(OpCode::LoadK, 3, one), 1);

            size_t loop = main.Here();
            main.Emit(EncodeABC(OpCode::Gt, 4, 1, 2), 2);
            size_t exit = main.Emit(EncodeAsBx(OpCode::JmpIfNot, 4, 0), 2);
            main.Emit(EncodeABC(OpCode::Add, 0, 0, 1), 3);
            main.Emit(EncodeABC(OpCode::Sub, 1, 1, 3), 3);
            size_t back = main.Emit(EncodeAsBx(OpCode::Jmp, 0, 0), 3);
            main.PatchJump(back, loop);
            main.PatchJump(exit, main.Here());

            main.Emit(EncodeABx(OpCode::GetGlobal, 5, 0), 4);
            main.Emit(EncodeABC(OpCode::Move, 6, 0), 4);
            main.Emit(EncodeABC(OpCode::Call, 5, 1), 4);
            main.Emit(EncodeABC(OpCode::Return, 0, 0), 4);

            std::string output;
            Check(RunProgram(program, heap, output), "loop runs");
            Check(output == "6\n", "loop adds 3 + 2 + 1");
        }

        // f crée une fermeture qui capture sa variable locale (42) et la renvoie. Quand f est terminée sa case de pile est
        // réutilisée par l'appel suivant, la fermeture ne doit donc plus la lire dans la pile mais dans l'upvalue fermée
        void TestVmClosedUpvalue()
        {
            Heap heap;
            CompiledProgram program;
            program.globalNames = { "afise" };
            program.main = std::make_unique<Prototype>();

            auto inner = std::make_unique<Prototype>();
            inner->name = "inner";
            inner->maxRegisters = 2;
            UpvalDesc captured;
            captured.fromParentRegister = true;
            captured.index = 0;
            inner->upvalues.push_back(captured);
            uint16_t seven = static_cast<uint16_t>(inner->AddConstant(Value::MakeNumber(7)));
            inner->Emit(EncodeABx(OpCode::LoadK, 0, seven), 1);    // écrase la case que l'upvalue ouverte désignerait
            inner->Emit(EncodeABC(OpCode::GetUpval, 1, 0), 1);
            inner->Emit(EncodeABC(OpCode::Return, 1, 1), 1);

            auto outer = std::make_unique<Prototype>();
            outer->name = "outer";
            outer->maxRegisters = 2;
            uint16_t answer = static_cast<uint16_t>(outer->AddConstant(Value::MakeNumber(42)));
            int32_t innerIndex = outer->AddProto(std::move(inner));
            outer->Emit(EncodeABx(OpCode::LoadK, 0, answer), 1);
            outer->Emit(EncodeABx(OpCode::Closure, 1, static_cast<uint16_t>(innerIndex)), 1);
            outer->Emit(EncodeABC(OpCode::Return, 1, 1), 1);

            Prototype& main = *program.main;
            main.maxRegisters = 4;
            int32_t outerIndex = main.AddProto(std::move(outer));
            main.Emit(EncodeABx(OpCode::Closure, 0, static_cast<uint16_t>(outerIndex)), 1);
            main.Emit(EncodeABC(OpCode::Move, 1, 0), 1);
            main.Emit(EncodeABC(OpCode::Call, 1, 0), 1);           // R1 = la fermeture
            main.Emit(EncodeABC(OpCode::Call, 1, 0), 1);           // R1 = 42
            main.Emit(EncodeABx(OpCode::GetGlobal, 2, 0), 2);
            main.Emit(EncodeABC(OpCode::Move, 3, 1), 2);
            main.Emit(EncodeABC(OpCode::Call, 2, 1), 2);
            main.Emit(EncodeABC(OpCode::Return, 0, 0), 2);

            std::string output;
            Check(RunProgram(program, heap, output), "closure program runs");
            Check(output == "42\n", "closed upvalue keeps its value after the frame is gone");
        }

        // Appeler autre chose qu'une fonction doit donner une erreur, pas un crash
        void TestVmRuntimeError()
        {
            Heap heap;
            CompiledProgram program;
            program.main = std::make_unique<Prototype>();

            Prototype& main = *program.main;
            main.maxRegisters = 1;
            uint16_t five = static_cast<uint16_t>(main.AddConstant(Value::MakeNumber(5)));
            main.Emit(EncodeABx(OpCode::LoadK, 0, five), 1);
            main.Emit(EncodeABC(OpCode::Call, 0, 0), 1);
            main.Emit(EncodeABC(OpCode::Return, 0, 0), 1);

            std::string output;
            Check(RunProgram(program, heap, output) == false, "calling a number is a runtime error");

            Check(RunProgram(CompiledProgram(), heap, output) == false, "an empty compilation result cannot run");
        }

        // addition(a, b) : keksoz c idon a èk b fwa 2 ... ran c
        void TestDisassembly()
        {
            Heap heap;

            auto fn = std::make_unique<Prototype>();
            fn->name = "addition";
            fn->numParams = 2;
            fn->maxRegisters = 4;
            int32_t two = fn->AddConstant(Value::MakeNumber(2));
            fn->Emit(EncodeABx(OpCode::LoadK, 3, static_cast<uint16_t>(two)), 4);
            fn->Emit(EncodeABC(OpCode::Mul, 3, 1, 3), 4);
            fn->Emit(EncodeABC(OpCode::Add, 2, 0, 3), 4);
            fn->Emit(EncodeABC(OpCode::Return, 2, 1), 5);

            Prototype main;
            main.maxRegisters = 3;
            std::vector<std::string> globals = { "afise" };
            int32_t hello = main.AddConstant(Value::MakeString(heap.Intern("Lebon")));
            int32_t protoIndex = main.AddProto(std::move(fn));
            main.Emit(EncodeABx(OpCode::Closure, 0, static_cast<uint16_t>(protoIndex)), 3);
            main.Emit(EncodeABx(OpCode::GetGlobal, 1, 0), 8);
            main.Emit(EncodeABx(OpCode::LoadK, 2, static_cast<uint16_t>(hello)), 8);
            main.Emit(EncodeABC(OpCode::Call, 1, 1), 8);
            size_t jump = main.Emit(EncodeAsBx(OpCode::Jmp, 0, 0), 9);
            main.Emit(EncodeABC(OpCode::LoadBool, 1, 1), 9);
            main.PatchJump(jump, main.Here());
            main.Emit(EncodeABC(OpCode::Return, 0, 0), 10);

            std::ostringstream out;
            Disassemble(main, out, globals);

            std::string expected =
                "== function <main> (params=0, registers=3) ==\n"
                "0000  L3   CLOSURE   R0 P0  ; addition\n"
                "0001  L8   GETGLOBAL R1 G0  ; afise\n"
                "0002  L8   LOADK     R2 K0  ; \"Lebon\"\n"
                "0003  L8   CALL      R1 1\n"
                "0004  L9   JMP       -> 0006\n"
                "0005  L9   LOADBOOL  R1 1\n"
                "0006  L10  RETURN    R0 0\n"
                "\n"
                "== function addition (params=2, registers=4) ==\n"
                "0000  L4   LOADK     R3 K0  ; 2\n"
                "0001  L4   MUL       R3 R1 R3\n"
                "0002  L4   ADD       R2 R0 R3\n"
                "0003  L5   RETURN    R2 1\n";

            if (out.str() != expected)
                std::cerr << "[bytecode] disassembly got:\n" << out.str() << "expected:\n" << expected;
            Check(out.str() == expected, "disassembly output");
        }
    }

    bool RunSelfTests()
    {
        g_failures = 0;
        TestEncoding();
        TestValues();
        TestConstantPool();
        TestJumpPatching();
        TestCompiledProgram();
        TestVmLoop();
        TestVmClosedUpvalue();
        TestVmRuntimeError();
        TestDisassembly();
        return g_failures == 0;
    }
}
