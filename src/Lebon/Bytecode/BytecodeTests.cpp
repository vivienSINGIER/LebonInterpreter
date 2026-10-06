#include "BytecodeTests.h"

#include <iostream>
#include <memory>
#include <sstream>

#include "Disassembler.h"
#include "Heap.hpp"
#include "Prototype.hpp"

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
        TestDisassembly();
        return g_failures == 0;
    }
}
