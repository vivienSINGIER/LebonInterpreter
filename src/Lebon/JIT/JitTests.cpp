#include "JitTests.h"

#include <bit>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Assembler.h"
#include "CodeGen.h"
#include "ExecutableMemory.h"
#include "Jit.hpp"
#include "Runtime.hpp"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Semantics/Analyser.h"
#include "core/Error.h"
#include "core/FileHelper.h"

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
            a.Adds();         emitted({ 0xF3, 0x0F, 0x58, 0xC1 }, "addss xmm0, xmm1");
            a.Subss();        emitted({ 0xF3, 0x0F, 0x5C, 0xC1 }, "subss xmm0, xmm1");
            a.Mulss();            emitted({ 0xF3, 0x0F, 0x59, 0xC1 }, "mulss xmm0, xmm1");
            a.Divss();            emitted({ 0xF3, 0x0F, 0x5E, 0xC1 }, "divss xmm0, xmm1");
            a.Xorps();            emitted({ 0x0F, 0x57, 0xC0 }, "xorps xmm0, xmm0");

            a.MovRspRax(8);       emitted({ 0x48, 0x89, 0x84, 0x24, 0x08, 0x00, 0x00, 0x00 }, "mov [rsp+d32], rax");
            a.MovssRspX0(8);      emitted({ 0xF3, 0x0F, 0x11, 0x84, 0x24, 0x08, 0x00, 0x00, 0x00 }, "movss [rsp+d32], xmm0");
            a.MovRcxRax();        emitted({ 0x48, 0x89, 0xC1 }, "mov rcx, rax");
            a.MovRdxRax();        emitted({ 0x48, 0x89, 0xC2 }, "mov rdx, rax");
            a.MovRcxRbp(-8);      emitted({ 0x48, 0x8B, 0x8D, 0xF8, 0xFF, 0xFF, 0xFF }, "mov rcx, [rbp+d32]");
            a.CallRax();          emitted({ 0xFF, 0xD0 }, "call rax");

            a.MovRcxRbpReg();     emitted({ 0x48, 0x89, 0xE9 }, "mov rcx, rbp");
            a.MovRcxArcxOff(16);  emitted({ 0x48, 0x8B, 0x89, 0x10, 0x00, 0x00, 0x00 }, "mov rcx, [rcx+d32]");
            a.MovRspRcx(8);       emitted({ 0x48, 0x89, 0x8C, 0x24, 0x08, 0x00, 0x00, 0x00 }, "mov [rsp+d32], rcx");
            a.MovssXmm0ArcxOff(-8); emitted({ 0xF3, 0x0F, 0x10, 0x81, 0xF8, 0xFF, 0xFF, 0xFF }, "movss xmm0, [rcx+d32]");
            a.MovssArcxOffXmm0(-8); emitted({ 0xF3, 0x0F, 0x11, 0x81, 0xF8, 0xFF, 0xFF, 0xFF }, "movss [rcx+d32], xmm0");
            a.MovRaxArcxOff(-8);  emitted({ 0x48, 0x8B, 0x81, 0xF8, 0xFF, 0xFF, 0xFF }, "mov rax, [rcx+d32]");
            a.MovArcxOffRax(-8);  emitted({ 0x48, 0x89, 0x81, 0xF8, 0xFF, 0xFF, 0xFF }, "mov [rcx+d32], rax");

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
        // _codeGenErrors receives how many errors the code generator reported, the code isn't run if there is one
        bool RunNumber(std::string const& _source, float& _out, size_t* _codeGenErrors = nullptr)
        {
            if (_codeGenErrors != nullptr)
                *_codeGenErrors = 0;

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
                size_t before = ErrorManager::Count();
                CodeGen codeGen(jit, program->stack.table);
                ok = codeGen.Run(*program) && jit.code.Entry() != nullptr && jit.entry < jit.code.Size();

                if (_codeGenErrors != nullptr)
                    *_codeGenErrors = ErrorManager::Count() - before;
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

        // Stage 2 : BinaryExpr and UnaryExpr on numbers.
        // The sources use the keywords without accents : azout, mwin, fwa, koup
        void TestCodeGenArithmetic()
        {
            auto gives = [](char const* _source, float _expected, char const* _what)
            {
                float result = 0.0f;
                Check(RunNumber(_source, result) && result == _expected, _what);
            };

            gives("1 azout 2", 3.0f, "addition");
            gives("10 mwin 4", 6.0f, "subtraction keeps the order of its operands");
            gives("6 fwa 7", 42.0f, "multiplication");
            gives("9 koup 2", 4.5f, "division keeps the order of its operands");

            gives("1 azout 2 fwa 3", 7.0f, "multiplication comes before addition");
            gives("(1 azout 2) fwa 3", 9.0f, "parentheses come first");
            gives("10 mwin 4 mwin 3", 3.0f, "subtraction groups from the left");
            gives("1 azout (2 azout (3 azout (4 azout 5)))", 15.0f, "nested right sides each keep their slot");
            gives("(1 azout 2) fwa (3 azout 4)", 21.0f, "both sides can be expressions");

            gives("mwin 5", -5.0f, "unary minus");
            gives("mwin mwin 5", 5.0f, "unary minus twice");
            gives("2 fwa mwin 3", -6.0f, "unary minus as a right side");
            gives("mwin (1 azout 2) mwin 4", -7.0f, "unary minus on an expression");

            float result = 0.0f;
            size_t errors = 0;
            Check(RunNumber("1 koup 0", result, &errors) == false && errors == 1, "division by a literal zero is reported");
            Check(RunNumber("0 koup 1", result, &errors) && errors == 0 && result == 0.0f, "zero can be divided");
            Check(RunNumber("1 koup 0.5", result, &errors) && errors == 0 && result == 2.0f, "a small divisor isn't reported");
        }

        // Stage 3 : VarDecl, Identifier and AssignExpr on globals
        void TestCodeGenGlobals()
        {
            auto gives = [](char const* _source, float _expected, char const* _what)
            {
                float result = 0.0f;
                Check(RunNumber(_source, result) && result == _expected, _what);
            };

            gives("keksoz a idon 2\na", 2.0f, "a global is read back");
            gives("keksoz a idon 2\na fwa 3", 6.0f, "a global is used in an expression");
            gives("keksoz a idon 10\nkeksoz b idon 4\na mwin b", 6.0f, "two globals have their own slot");
            gives("keksoz a idon 3\nkeksoz b idon a fwa a\nb", 9.0f, "a global is initialized from another one");

            gives("keksoz a idon 2\na idon 5\na", 5.0f, "an assignment replaces the value");
            gives("keksoz a idon 2\na idon a azout 1\na fwa 3", 9.0f, "an assignment can read the variable it writes");
            gives("keksoz a idon 2\na idon 7", 7.0f, "an assignment gives its value");
            gives("keksoz a idon 1\nkeksoz b idon (a idon 5) azout 1\na azout b", 11.0f, "an assignment is used inside an expression");

            gives("keksoz a\na idon 4\na azout 1", 5.0f, "a global declared without a value is assigned later");
        }

        // Runs the source like RunNumber, _out receives what it printed instead of the console
        bool RunOutput(std::string const& _source, std::string& _out)
        {
            ::Runtime::BufferSink captured;
            ::Runtime::OutputSink* console = Runtime::g_out;
            Runtime::g_out = &captured;

            float ignored = 0.0f;
            bool ok = RunNumber(_source, ignored);

            Runtime::g_out = console;
            _out = captured.Str();
            return ok;
        }

        // Stage 4 : CallExpr on afise
        void TestCodeGenPrint()
        {
            auto prints = [](char const* _source, char const* _expected, char const* _what)
            {
                std::string output;
                Check(RunOutput(_source, output) && output == _expected, _what);
            };

            prints("afise(42)", "42\n", "a number is printed");
            prints("afise(4.5)", "4.5\n", "a number is printed with its decimals");
            prints("afise(mwin 2.5)", "-2.5\n", "a negative number is printed");
            prints("afise(1 azout 2 fwa 3)", "7\n", "an expression is printed");
            prints("keksoz a idon 2\nafise(a fwa 3)", "6\n", "a global is printed");

            prints("afise(1)\nafise(2)\nafise(3)", "1\n2\n3\n", "several calls print in order");
            prints("keksoz a idon 5\nafise(a)\na idon a azout 1\nafise(a)", "5\n6\n", "a global keeps its value after a call");
        }

        // Stage 5 : FuncDecl, ReturnStmt, Block, CallExpr on Lebon functions, params and locals
        void TestCodeGenFunctions()
        {
            auto gives = [](std::string const& _source, float _expected, char const* _what)
            {
                float result = 0.0f;
                Check(RunNumber(_source, result) && result == _expected, _what);
            };
            auto prints = [](std::string const& _source, char const* _expected, char const* _what)
            {
                std::string output;
                Check(RunOutput(_source, output) && output == _expected, _what);
            };

            std::string const add = "zafer add(a, b)\nouver\n    ran a azout b\nlafin\n";
            std::string const sub = "zafer sub(a, b)\nouver\n    ran a mwin b\nlafin\n";

            gives("zafer sis()\nouver\n    ran 6\nlafin\nsis()", 6.0f, "a function returns a value");
            gives(add + "add(2, 3)", 5.0f, "a function receives its arguments");
            gives(sub + "sub(10, 4)", 6.0f, "the arguments keep their order");
            gives("zafer f()\nouver\n    ran 1\n    ran 2\nlafin\nf()", 1.0f, "a return leaves the function");

            gives(add + "add(add(1, 2), add(3, 4))", 10.0f, "calls are used as arguments");
            gives(add + "1 azout add(2, 3) fwa 2", 11.0f, "a call is used inside an expression");
            gives(sub + "sub(100, sub(50, sub(20, 5)))", 65.0f, "calls are nested on the right");
            gives("zafer f(a, b, c, d, e, g)\nouver\n    ran a mwin b fwa c azout d koup e mwin g\nlafin\nf(50, 2, 3, 8, 4, 1)", 45.0f,
                "a function receives more than four arguments");

            gives("zafer f(a, b)\nouver\n    keksoz s idon a azout b\n    keksoz d idon a mwin b\n    ran s fwa d\nlafin\nf(5, 3)", 16.0f,
                "a function has its own locals");
            gives("zafer f(a)\nouver\n    keksoz s\n    s idon a fwa 2\n    ran s\nlafin\nf(4)", 8.0f, "a local declared without a value is assigned later");
            gives("zafer f(a)\nouver\n    a idon a azout 1\n    ran a\nlafin\nf(4)", 5.0f, "a param can be assigned");
            gives("zafer f(a)\nouver\n    keksoz r idon a\n    ouver\n        keksoz t idon a fwa 2\n        r idon r azout t\n    lafin\n"
                "    ouver\n        keksoz u idon 100\n        r idon r azout u\n    lafin\n    ran r\nlafin\nf(3)", 109.0f, "blocks have their own locals");

            gives("keksoz g idon 10\nzafer f(a)\nouver\n    ran a fwa g\nlafin\nf(3)", 30.0f, "a function reads a global");
            gives("keksoz g idon 10\nzafer f(a)\nouver\n    g idon a\nlafin\nf(3)\ng", 3.0f, "a function writes a global");
            gives(add + "zafer dub(a)\nouver\n    ran add(a, a)\nlafin\ndub(21)", 42.0f, "a function calls another one");
            gives("keksoz x idon 2\n" + add + "keksoz y idon add(x, 3)\nx fwa y", 10.0f, "the top level goes on after a call");

            prints("zafer show(a)\nouver\n    afise(a)\n    afise(a fwa 2)\nlafin\nshow(4)\nshow(5)", "4\n8\n5\n10\n",
                "a function without a return prints");

            // Nested functions
            gives("zafer f(a)\nouver\n    zafer in(b)\n    ouver\n        ran b fwa 2\n    lafin\n    ran in(a) azout 1\nlafin\nf(4)", 9.0f,
                "a nested function is called by its parent");
            gives("zafer f(a)\nouver\n    keksoz k idon 5\n    zafer in(b)\n    ouver\n        ran b mwin k\n    lafin\n    ran in(a)\nlafin\nf(30)", 25.0f,
                "a nested function reads a local of its parent");
            gives("zafer f(a, c)\nouver\n    zafer in(b)\n    ouver\n        ran b azout a fwa c\n    lafin\n    ran in(1)\nlafin\nf(4, 10)", 41.0f,
                "a nested function reads the params of its parent");
            gives("zafer f()\nouver\n    keksoz k idon 1\n    zafer set(v)\n    ouver\n        k idon v\n    lafin\n    set(7)\n    ran k\nlafin\nf()", 7.0f,
                "a nested function writes a local of its parent");
            gives("zafer f(a)\nouver\n    keksoz k idon 3\n    zafer mid(b)\n    ouver\n        keksoz m idon 10\n        zafer in(c)\n        ouver\n"
                "            ran c azout m azout k azout a\n        lafin\n        ran in(b)\n    lafin\n    ran mid(200)\nlafin\nf(1000)", 1213.0f,
                "a function nested twice reads both of its parents");
            gives("zafer f()\nouver\n    keksoz k idon 4\n    zafer one(b)\n    ouver\n        ran b fwa k\n    lafin\n    zafer two(b)\n    ouver\n"
                "        ran one(b) azout 1\n    lafin\n    ran two(5)\nlafin\nf()", 21.0f, "a nested function calls the one declared next to it");
            gives("keksoz g idon 2\nzafer top(a)\nouver\n    ran a fwa g\nlafin\nzafer f(a)\nouver\n    zafer in(b)\n    ouver\n        ran top(b) azout 1\n    lafin\n"
                "    ran in(a)\nlafin\nf(5)", 11.0f, "a nested function calls a function of the top level");
            prints("ouver\n    keksoz k idon 7\n    zafer get(a)\n    ouver\n        ran k azout a\n    lafin\n    afise(get(1))\n    k idon 20\n    afise(get(1))\nlafin",
                "8\n21\n", "a function reads a local of the top level block it is declared in");
        }

        // Stage 6 : StringLiteral, BooleanLiteral and the addition of strings.
        // pafo and fo are the booleans without accents
        void TestCodeGenStrings()
        {
            auto prints = [](std::string const& _source, char const* _expected, char const* _what)
            {
                std::string output;
                Check(RunOutput(_source, output) && output == _expected, _what);
            };

            // Literals
            prints("afise(\"Lebon\")", "Lebon\n", "a string is printed");
            prints("afise(\"\")", "\n", "an empty string is printed");
            prints("afise(\"S\xC3\xA9 Lebon\")", "S\xC3\xA9 Lebon\n", "a string keeps its accents");
            prints("afise(pafo)", "true\n", "true is printed");
            prints("afise(fo)", "false\n", "false is printed");

            // Addition of strings
            prints("afise(\"left\" azout \"right\")", "leftright\n", "two strings are added in order");
            prints("afise(\"a\" azout \"b\" azout \"c\" azout \"d\")", "abcd\n", "several strings are added");
            prints("afise(\"a\" azout (\"b\" azout (\"c\" azout \"d\")))", "abcd\n", "strings are added on the right first");
            prints("afise((\"a\" azout \"b\") azout (\"c\" azout \"d\"))", "abcd\n", "both sides can be additions");
            prints("afise(\"\" azout \"x\" azout \"\")", "x\n", "an empty string adds nothing");
            prints("afise(\"same\" azout \"same\")", "samesame\n", "a string is added to itself");

            // Variables
            prints("keksoz s idon \"Nathan\"\nafise(s)", "Nathan\n", "a global holds a string");
            prints("keksoz s idon \"a\"\ns idon s azout \"b\"\ns idon s azout s\nafise(s)", "abab\n", "a string global is assigned");
            prints("keksoz s idon \"a\"\nkeksoz t idon s azout \"b\"\nafise(s)\nafise(t)", "a\nab\n", "adding to a string leaves it unchanged");
            prints("keksoz b idon pafo\nafise(b)\nb idon fo\nafise(b)", "true\nfalse\n", "a global holds a boolean");
            prints("keksoz s idon \"x\"\nkeksoz n idon 2\nkeksoz b idon pafo\nafise(s)\nafise(n)\nafise(b)", "x\n2\ntrue\n",
                "globals of each type sit next to each other");
            prints("ouver\n    keksoz s idon \"in\"\n    ouver\n        keksoz t idon s azout \"ner\"\n        afise(t)\n    lafin\n    afise(s)\nlafin",
                "inner\nin\n", "a local of a block holds a string");

            // Functions
            std::string const tit = "zafer tit(non)\nouver\n    ran \"Kliyan : \" azout non\nlafin\n";

            prints(tit + "afise(tit(\"Nathan\"))", "Kliyan : Nathan\n", "a function receives and returns a string");
            prints(tit + "keksoz k idon \"Alice\"\nafise(tit(k))\nafise(k)", "Kliyan : Alice\nAlice\n", "a string global is passed to a function");
            prints(tit + "afise(tit(\"a\") azout tit(\"b\"))", "Kliyan : aKliyan : b\n", "the results of two calls are added");
            prints(tit + "afise(tit(tit(\"x\")))", "Kliyan : Kliyan : x\n", "a call on a string is used as an argument");
            prints("zafer f(a)\nouver\n    keksoz s idon a azout \"!\"\n    keksoz t idon s azout s\n    ran t\nlafin\nafise(f(\"ho\"))", "ho!ho!\n",
                "a function has string locals");
            prints("zafer wi()\nouver\n    ran pafo\nlafin\nzafer non()\nouver\n    ran fo\nlafin\nafise(wi())\nafise(non())", "true\nfalse\n",
                "a function returns a boolean");
            prints("zafer same(b)\nouver\n    ran b\nlafin\nafise(same(fo))\nafise(same(pafo))", "false\ntrue\n", "a function receives a boolean");
            prints("zafer show(s, n, b, t)\nouver\n    afise(s)\n    afise(n)\n    afise(b)\n    afise(t)\nlafin\nshow(\"one\", 2.5, pafo, \"four\")",
                "one\n2.5\ntrue\nfour\n", "arguments of each type keep their place");
            prints("zafer wrap(a, b, c, d, e)\nouver\n    ran a azout b azout c azout d azout e\nlafin\nafise(wrap(\"1\", \"2\", \"3\", \"4\", \"5\"))",
                "12345\n", "a function receives more than four strings");

            // Nested functions
            prints("zafer f(a)\nouver\n    keksoz pre idon \"<\"\n    zafer in(b)\n    ouver\n        ran pre azout b azout a\n    lafin\n    ran in(\"-\")\nlafin\n"
                "afise(f(\">\"))", "<->\n", "a nested function reads the strings of its parent");
            prints("zafer f()\nouver\n    keksoz s idon \"old\"\n    keksoz b idon fo\n    zafer set(v)\n    ouver\n        s idon v\n        b idon pafo\n    lafin\n"
                "    set(\"new\")\n    afise(b)\n    ran s\nlafin\nafise(f())", "true\nnew\n", "a nested function writes a string and a boolean of its parent");
        }

        // The demo program, read from the place main reads it
        void TestCodeGenProgram()
        {
            std::string source;
            if (FileHelper::ReadFile("../../res/Lebon/tests/valid/program.lbn", source))
            {
                std::cerr << "[jit] program.lbn not found from the working directory, its test is skipped\n";
                return;
            }

            std::string output;
            Check(RunOutput(source, output), "the demo program is compiled and run");
            Check(output == "S\xC3\xA9 Lebon\nKliyan : Nathan\n9.765\n7.812\n3.255\n18.832\n6.944\n", "the demo program prints its receipt");
        }

        // Conditions : the comparisons and IfStmt.
        // The sources use the keywords without accents : kan, otreman, sinon-si,
        // parey, diferan, piti, plis-gran, pa-gran (lower or equal), pa-piti (greater or equal)
        void TestCodeGenConditions()
        {
            auto prints = [](std::string const& _source, char const* _expected, char const* _what)
            {
                std::string output;
                Check(RunOutput(_source, output) && output == _expected, _what);
            };

            // Each comparison is tried below, on and above its limit
            prints("afise(1 piti 2)\nafise(2 piti 2)\nafise(3 piti 2)", "true\nfalse\nfalse\n", "lower");
            prints("afise(1 plis-gran 2)\nafise(2 plis-gran 2)\nafise(3 plis-gran 2)", "false\nfalse\ntrue\n", "greater");
            prints("afise(1 pa-gran 2)\nafise(2 pa-gran 2)\nafise(3 pa-gran 2)", "true\ntrue\nfalse\n", "lower or equal");
            prints("afise(1 pa-piti 2)\nafise(2 pa-piti 2)\nafise(3 pa-piti 2)", "false\ntrue\ntrue\n", "greater or equal");
            prints("afise(1 parey 2)\nafise(2 parey 2)", "false\ntrue\n", "equal");
            prints("afise(1 diferan 2)\nafise(2 diferan 2)", "true\nfalse\n", "not equal");

            prints("afise(mwin 5 piti 3)\nafise(3 piti mwin 5)\nafise(mwin 1 piti mwin 2)\nafise(mwin 2 piti mwin 1)", "true\nfalse\nfalse\ntrue\n",
                "negative numbers are ordered");
            prints("afise(0.5 piti 0.75)\nafise(0.75 piti 0.5)\nafise(2.5 parey 2.5)", "true\nfalse\ntrue\n", "decimals are compared");
            prints("afise(1 azout 2 parey 3)\nafise(2 fwa 3 plis-gran 10 mwin 5)", "true\ntrue\n", "arithmetic comes before a comparison");
            prints("keksoz a idon 4\nkeksoz b idon 9\nafise(a piti b)\nafise(b piti a)\nafise(a parey a)", "true\nfalse\ntrue\n", "variables are compared");
            prints("keksoz b idon 3 piti 4\nafise(b)\nb idon 4 piti 3\nafise(b)", "true\nfalse\n", "a comparison is kept in a variable");

            // Strings and booleans
            prints("afise(\"a\" parey \"a\")\nafise(\"a\" parey \"b\")\nafise(\"a\" diferan \"b\")\nafise(\"a\" diferan \"a\")", "true\nfalse\ntrue\nfalse\n",
                "strings are compared");
            prints("afise((\"a\" azout \"b\") parey \"ab\")\nafise((\"a\" azout \"b\") parey (\"a\" azout \"c\"))", "true\nfalse\n",
                "an added string is equal to the same text");
            prints("keksoz s idon \"x\"\nkeksoz t idon s azout \"y\"\nafise(t parey \"xy\")\nafise(s parey t)", "true\nfalse\n", "string variables are compared");
            prints("afise(pafo parey pafo)\nafise(pafo parey fo)\nafise(pafo diferan fo)\nafise(fo diferan fo)", "true\nfalse\ntrue\nfalse\n",
                "booleans are compared");
            prints("afise((1 piti 2) parey pafo)\nafise((2 piti 1) parey pafo)\nafise((1 piti 2) parey (3 piti 4))", "true\nfalse\ntrue\n",
                "the result of a comparison is a clean boolean");

            // If
            prints("kan 1 piti 2\nouver\n    afise(1)\nlafin\nafise(9)", "1\n9\n", "a true condition runs its block");
            prints("kan 2 piti 1\nouver\n    afise(1)\nlafin\nafise(9)", "9\n", "a false condition skips its block");
            prints("kan pafo\nouver\n    afise(1)\nlafin\nkan fo\nouver\n    afise(2)\nlafin", "1\n", "a boolean is a condition");
            prints("keksoz b idon 5 plis-gran 3\nkan b\nouver\n    afise(1)\nlafin", "1\n", "a boolean variable is a condition");
            prints("kan 1 piti 2\nouver\n    afise(1)\nlafin\notreman\nouver\n    afise(2)\nlafin\nafise(9)", "1\n9\n", "a true condition skips the else");
            prints("kan 2 piti 1\nouver\n    afise(1)\nlafin\notreman\nouver\n    afise(2)\nlafin\nafise(9)", "2\n9\n", "a false condition runs the else");
            prints("kan \"a\" parey \"a\"\nouver\n    afise(\"same\")\nlafin\notreman\nouver\n    afise(\"other\")\nlafin", "same\n", "a string comparison is a condition");

            // One function for the three paths of a chain
            std::string const sign = "zafer sign(n)\nouver\n    kan n piti 0\n    ouver\n        ran \"neg\"\n    lafin\n    sinon-si n parey 0\n    ouver\n"
                "        ran \"zero\"\n    lafin\n    otreman\n    ouver\n        ran \"pos\"\n    lafin\nlafin\n";
            prints(sign + "afise(sign(mwin 3))\nafise(sign(0))\nafise(sign(8))", "neg\nzero\npos\n", "a chain picks one of its blocks");

            std::string const rank = "zafer rank(n)\nouver\n    kan n piti 10\n    ouver\n        ran 1\n    lafin\n    sinon-si n piti 20\n    ouver\n        ran 2\n    lafin\n"
                "    sinon-si n piti 30\n    ouver\n        ran 3\n    lafin\n    ran 4\nlafin\n";
            prints(rank + "afise(rank(5))\nafise(rank(15))\nafise(rank(25))\nafise(rank(35))", "1\n2\n3\n4\n", "a chain without an else falls out");

            prints("keksoz a idon 5\nkan a plis-gran 0\nouver\n    kan a plis-gran 3\n    ouver\n        afise(1)\n    lafin\n    otreman\n    ouver\n        afise(2)\n    lafin\n"
                "    afise(3)\nlafin\notreman\nouver\n    afise(4)\nlafin\nafise(9)", "1\n3\n9\n", "conditions are nested");

            // Variables and blocks
            prints("keksoz a idon 1\nkan a parey 1\nouver\n    a idon 10\nlafin\nkan a parey 1\nouver\n    a idon 20\nlafin\nafise(a)", "10\n",
                "a block assigns a variable declared outside");
            prints("zafer f(n)\nouver\n    keksoz r idon 0\n    kan n plis-gran 0\n    ouver\n        keksoz t idon n fwa 2\n        r idon t azout 1\n    lafin\n    otreman\n    ouver\n"
                "        keksoz u idon n mwin 100\n        r idon u\n    lafin\n    keksoz z idon 1000\n    ran r azout z\nlafin\nafise(f(4))\nafise(f(mwin 4))",
                "1009\n896\n", "each block has its own locals");
            prints("kan 1 piti 2\nouver\n    afise(1)\n    afise(2)\n    afise(3)\n    afise(4)\n    afise(5)\n    afise(6)\n    afise(7)\n    afise(8)\n    afise(9)\n    afise(10)\n"
                "    afise(11)\n    afise(12)\nlafin\nafise(0)", "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n0\n", "a long block is run in full");
            prints("kan 2 piti 1\nouver\n    afise(1)\n    afise(2)\n    afise(3)\n    afise(4)\n    afise(5)\n    afise(6)\n    afise(7)\n    afise(8)\n    afise(9)\n    afise(10)\n"
                "    afise(11)\n    afise(12)\nlafin\nafise(0)", "0\n", "a long block is skipped in full");

            // Functions
            prints("zafer abs(n)\nouver\n    kan n piti 0\n    ouver\n        ran mwin n\n    lafin\n    ran n\nlafin\nafise(abs(mwin 7))\nafise(abs(7))\nafise(abs(0))",
                "7\n7\n0\n", "a return inside a block leaves the function");
            prints("zafer max(a, b)\nouver\n    kan a plis-gran b\n    ouver\n        ran a\n    lafin\n    otreman\n    ouver\n        ran b\n    lafin\nlafin\n"
                "afise(max(3, 8))\nafise(max(8, 3))\nafise(max(max(1, 9), max(4, 2)))", "8\n8\n9\n", "both blocks of a function return");
            prints("zafer even(n)\nouver\n    ran n koup 2 fwa 2 parey n\nlafin\nkan even(4)\nouver\n    afise(\"yes\")\nlafin", "yes\n", "a call is a condition");
            prints("zafer f(lim)\nouver\n    zafer over(n)\n    ouver\n        kan n plis-gran lim\n        ouver\n            ran pafo\n        lafin\n        ran fo\n    lafin\n"
                "    afise(over(5))\n    afise(over(50))\nlafin\nf(10)", "false\ntrue\n", "a nested function tests a variable of its parent");

            // Recursion, which needs a condition to stop
            prints("zafer fakt(n)\nouver\n    kan n pa-gran 1\n    ouver\n        ran 1\n    lafin\n    ran n fwa fakt(n mwin 1)\nlafin\nafise(fakt(1))\nafise(fakt(5))\nafise(fakt(10))",
                "1\n120\n3.6288e+06\n", "a function calls itself");
            prints("zafer fib(n)\nouver\n    kan n piti 2\n    ouver\n        ran n\n    lafin\n    ran fib(n mwin 1) azout fib(n mwin 2)\nlafin\nafise(fib(10))\nafise(fib(20))",
                "55\n6765\n", "a function calls itself twice");
            prints("zafer som(n)\nouver\n    kan n parey 0\n    ouver\n        ran 0\n    lafin\n    ran n azout som(n mwin 1)\nlafin\nafise(som(100))\nafise(som(1000))",
                "5050\n500500\n", "a function calls itself a thousand times");
            prints("zafer rep(s, n)\nouver\n    kan n parey 0\n    ouver\n        ran \"\"\n    lafin\n    ran s azout rep(s, n mwin 1)\nlafin\nafise(rep(\"ab\", 3))",
                "ababab\n", "a function calling itself builds a string");
            prints("zafer f(n)\nouver\n    keksoz acc idon 0\n    zafer walk(i)\n    ouver\n        kan i plis-gran n\n        ouver\n            ran\n        lafin\n        acc idon acc azout i\n"
                "        walk(i azout 1)\n    lafin\n    walk(1)\n    ran acc\nlafin\nafise(f(10))", "55\n", "a nested function calling itself keeps the frame of its parent");
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
            TestCodeGenArithmetic();
            TestCodeGenGlobals();
            TestCodeGenPrint();
            TestCodeGenFunctions();
            TestCodeGenStrings();
            TestCodeGenProgram();
            TestCodeGenConditions();
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
