#include "JitTests.h"

#include <bit>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Assembler.h"
#include "CodeGen.h"
#include "ExecutableMemory.h"
#include "Jit.hpp"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Semantics/Analyser.h"
#include "core/Error.h"

namespace Jit
{
    namespace
    {
        uint32_t g_failures = 0;

        void Check(bool _ok, char const* _what)
        {
            if (_ok)
                return;
            std::cerr << "[jit] FAIL " << _what << "\n";
            g_failures++;
        }

        // The bytes are compared with what a real assembler gives for the same instruction
        void TestEncoding()
        {
            Assembler mov32;
            mov32.MovEaxImm32(42);
            Check(mov32.Code() == std::vector<uint8_t>{ 0xB8, 0x2A, 0x00, 0x00, 0x00 }, "mov eax, imm32");

            Assembler mov64;
            mov64.MovRaxImm64(0x1122334455667788);
            Check(mov64.Code() == std::vector<uint8_t>{ 0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 }, "mov rax, imm64");

            Assembler ret;
            ret.Ret();
            Check(ret.Code() == std::vector<uint8_t>{ 0xC3 }, "ret");

            using Bytes = std::vector<uint8_t>;
            Assembler a;
            size_t start = 0;
            // Checks what was emitted since the previous call
            auto emitted = [&](Bytes const& _expected, char const* _what)
            {
                Bytes got(a.Code().begin() + start, a.Code().end());
                Check(got == _expected, _what);
                start = a.Size();
            };

            a.PushRbp();          emitted({ 0x55 }, "push rbp");
            a.MovRbpRsp();        emitted({ 0x48, 0x89, 0xE5 }, "mov rbp, rsp");
            a.SubRspImm32(48);    emitted({ 0x48, 0x81, 0xEC, 0x30, 0x00, 0x00, 0x00 }, "sub rsp, imm32");
            a.MovRspRbp();        emitted({ 0x48, 0x89, 0xEC }, "mov rsp, rbp");
            a.PopRbp();           emitted({ 0x5D }, "pop rbp");

            a.MovdXmm0Eax();      emitted({ 0x66, 0x0F, 0x6E, 0xC0 }, "movd xmm0, eax");

            a.MovRaxRbp(-8);      emitted({ 0x48, 0x8B, 0x85, 0xF8, 0xFF, 0xFF, 0xFF }, "mov rax, [rbp+d32]");
            a.MovRbpRax(-8);      emitted({ 0x48, 0x89, 0x85, 0xF8, 0xFF, 0xFF, 0xFF }, "mov [rbp+d32], rax");
            a.MovssXmm0Rbp(16);   emitted({ 0xF3, 0x0F, 0x10, 0x85, 0x10, 0x00, 0x00, 0x00 }, "movss xmm0, [rbp+d32]");
            a.MovssRbpXmm0(-8);   emitted({ 0xF3, 0x0F, 0x11, 0x85, 0xF8, 0xFF, 0xFF, 0xFF }, "movss [rbp+d32], xmm0");

            a.MovRcxImm64(0x1122334455667788);
            emitted({ 0x48, 0xB9, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 }, "mov rcx, imm64");
            a.MovRaxArcx();       emitted({ 0x48, 0x8B, 0x01 }, "mov rax, [rcx]");
            a.MovArcxRax();       emitted({ 0x48, 0x89, 0x01 }, "mov [rcx], rax");
            a.MovssXmm0Arcx();    emitted({ 0xF3, 0x0F, 0x10, 0x01 }, "movss xmm0, [rcx]");
            a.MovssArcxXmm0();    emitted({ 0xF3, 0x0F, 0x11, 0x01 }, "movss [rcx], xmm0");

            a.MovapsX1X0();       emitted({ 0x0F, 0x28, 0xC8 }, "movaps xmm1, xmm0");
            a.AddsX0X1();         emitted({ 0xF3, 0x0F, 0x58, 0xC1 }, "addss xmm0, xmm1");
            a.SubssX0X1();        emitted({ 0xF3, 0x0F, 0x5C, 0xC1 }, "subss xmm0, xmm1");
            a.Mulss();            emitted({ 0xF3, 0x0F, 0x59, 0xC1 }, "mulss xmm0, xmm1");
            a.Divss();            emitted({ 0xF3, 0x0F, 0x5E, 0xC1 }, "divss xmm0, xmm1");
            a.Xorps();            emitted({ 0x0F, 0x57, 0xC0 }, "xorps xmm0, xmm0");

            a.MovRspRax(8);       emitted({ 0x48, 0x89, 0x84, 0x24, 0x08, 0x00, 0x00, 0x00 }, "mov [rsp+d32], rax");
            a.MovssRspX0(8);      emitted({ 0xF3, 0x0F, 0x11, 0x84, 0x24, 0x08, 0x00, 0x00, 0x00 }, "movss [rsp+d32], xmm0");
            a.MovRcxRax();        emitted({ 0x48, 0x89, 0xC1 }, "mov rcx, rax");
            a.MovRdxRax();        emitted({ 0x48, 0x89, 0xC2 }, "mov rdx, rax");
            a.MovRcxRbp(-8);      emitted({ 0x48, 0x8B, 0x8D, 0xF8, 0xFF, 0xFF, 0xFF }, "mov rcx, [rbp+d32]");
            a.CallRax();          emitted({ 0xFF, 0xD0 }, "call rax");

            // A call is 5 bytes long, the distance is counted from its end
            Assembler back;
            back.Ret();
            back.Callr32(0);
            Check(back.Code() == Bytes{ 0xC3, 0xE8, 0xFA, 0xFF, 0xFF, 0xFF }, "call rel32 backward");

            Assembler forth;
            forth.Callr32(8);
            Check(forth.Code() == Bytes{ 0xE8, 0x03, 0x00, 0x00, 0x00 }, "call rel32 forward");
        }

#ifdef _M_X64
        // 6 fwa 7, with a real frame
        void TestArithmetic()
        {
            Assembler a;
            a.PushRbp();
            a.MovRbpRsp();
            a.SubRspImm32(16);
            a.MovEaxImm32(std::bit_cast<uint32_t>(6.0f));
            a.MovdXmm0Eax();
            a.MovssRbpXmm0(-8);
            a.MovEaxImm32(std::bit_cast<uint32_t>(7.0f));
            a.MovdXmm0Eax();
            a.MovapsX1X0();
            a.MovssXmm0Rbp(-8);
            a.Mulss();
            a.MovRspRbp();
            a.PopRbp();
            a.Ret();

            ExecutableMemory code;
            if (code.Load(a.Code()))
                Check(code.As<float (*)()>()() == 42.0f, "generated code multiplies two numbers");
        }

        // A function at offset 0, then a caller that reaches it with a relative call
        void TestCall()
        {
            Assembler a;
            a.MovEaxImm32(42);
            a.Ret();

            size_t caller = a.Size();
            a.PushRbp();
            a.MovRbpRsp();
            a.Callr32(0);
            a.MovRspRbp();
            a.PopRbp();
            a.Ret();

            ExecutableMemory code;
            if (code.Load(a.Code()))
            {
                void* entry = static_cast<uint8_t*>(code.Entry()) + caller;
                Check(reinterpret_cast<int (*)()>(entry)() == 42, "generated code calls another generated function");
            }
        }
#endif

#ifdef _M_X64
        // Compiles the source then runs its top level code, false if a stage failed.
        // The value of the last expression is still in xmm0 when the code returns, this is what _out receives
        bool RunNumber(std::string const& _source, float& _out)
        {
            Lexer lexer(_source);
            lexer.Scan();

            Parser parser(lexer.GetTokens());
            std::unique_ptr<Program> program = parser.Parse();
            bool ok = program != nullptr && ErrorManager::HasErrors() == false;

            if (ok)
            {
                Semantics::Analyser analyser;
                ok = analyser.Run(*program);
            }

            JitCode jit;
            if (ok)
            {
                CodeGen codeGen(jit, program->stack.table);
                ok = codeGen.Run(*program) && jit.code.Entry() != nullptr && jit.entry < jit.code.Size();
            }

            if (ok)
            {
                void* entry = static_cast<uint8_t*>(jit.code.Entry()) + jit.entry;
                _out = reinterpret_cast<float (*)()>(entry)();
            }

            ErrorManager::Clear();
            return ok;
        }

        // Stage 1 : Program, ExprStmt and NumberLiteral
        void TestCodeGenLiterals()
        {
            float result = 0.0f;

            Check(RunNumber("42", result), "a number literal is compiled");
            Check(result == 42.0f, "a number literal is returned");

            result = 0.0f;
            Check(RunNumber("4.5", result) && result == 4.5f, "a number literal keeps its decimals");

            result = 0.0f;
            Check(RunNumber("7\n42\n", result) && result == 42.0f, "the last statement gives the value");
        }
#endif

        void TestExecution()
        {
#ifdef _M_X64
            Assembler small;
            small.MovEaxImm32(42);
            small.Ret();

            ExecutableMemory smallCode;
            Check(smallCode.Load(small.Code()), "executable memory is allocated");
            if (smallCode.Entry() != nullptr)
                Check(smallCode.As<int (*)()>()() == 42, "generated code returns 42");

            Assembler wide;
            wide.MovRaxImm64(0x1122334455667788);
            wide.Ret();

            ExecutableMemory wideCode;
            Check(wideCode.Load(wide.Code()), "executable memory is allocated twice");
            if (wideCode.Entry() != nullptr)
                Check(wideCode.As<uint64_t (*)()>()() == 0x1122334455667788, "generated code returns a 64 bits value");

            ExecutableMemory empty;
            Check(empty.Load({}) == false && empty.Entry() == nullptr, "empty code isn't loaded");

            TestArithmetic();
            TestCall();
            TestCodeGenLiterals();
#else
            std::cerr << "[jit] execution tests skipped, the generated code is x64 only\n";
#endif
        }
    }

    bool RunSelfTests()
    {
        g_failures = 0;

        TestEncoding();
        TestExecution();

        if (g_failures == 0)
            std::cout << "[jit] self tests passed\n";
        return g_failures == 0;
    }
}
