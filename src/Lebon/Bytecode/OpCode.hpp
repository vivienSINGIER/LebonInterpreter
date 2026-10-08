#ifndef OPCODE_HPP_INCLUDED
#define OPCODE_HPP_INCLUDED

#include <cstdint>

namespace Bytecode
{
    using Instruction = uint32_t;

    enum class OpCode : uint8_t
    {
        Move,       // A B      R[A] = R[B]
        LoadK,      // A Bx     R[A] = K[Bx]
        LoadBool,   // A B      R[A] = (B != 0)
        LoadNil,    // A        R[A] = nil

        GetGlobal,  // A Bx     R[A] = Globals[Bx]
        SetGlobal,  // A Bx     Globals[Bx] = R[A]
        GetUpval,   // A B      R[A] = Upvalues[B]
        SetUpval,   // A B      Upvalues[B] = R[A]

        Add,        // A B C    R[A] = R[B] + R[C]  (numbers only, the compiler picks Concat for strings)
        Concat,     // A B C    R[A] = R[B] .. R[C] (strings only)
        Sub,       // A B C    R[A] = R[B] - R[C]
        Mul,        // A B C    R[A] = R[B] * R[C]
        Div,        // A B C    R[A] = R[B] / R[C]
        Neg,        // A B      R[A] = -R[B]
        Not,        // A B      R[A] = not R[B]

        Eq,         // A B C    R[A] = (R[B] == R[C])
        Lt,         // A B C    R[A] = (R[B] <  R[C])
        Le,         // A B C    R[A] = (R[B] <= R[C])
        Gt,         // A B C    R[A] = (R[B] >  R[C])
        Ge,         // A B C    R[A] = (R[B] >= R[C])

        // Même opérations avec une constante comme second opérande : C est l'index de la constante (0 à 255).
        // Elles évitent un LOADK avant chaque opération avec un littéral
        AddK,       // A B C    R[A] = R[B] + K[C]
        ConcatK,    // A B C    R[A] = R[B] .. K[C]
        SubK,       // A B C    R[A] = R[B] - K[C]
        MulK,       // A B C    R[A] = R[B] * K[C]
        DivK,       // A B C    R[A] = R[B] / K[C]
        EqK,        // A B C    R[A] = (R[B] == K[C])
        LtK,        // A B C    R[A] = (R[B] <  K[C])
        LeK,        // A B C    R[A] = (R[B] <= K[C])
        GtK,        // A B C    R[A] = (R[B] >  K[C])
        GeK,        // A B C    R[A] = (R[B] >= K[C])

        Jmp,       // sBx      pc += sBx
        JmpIfNot,   // A sBx    if R[A] is false (or nil) then pc += sBx

        Call,       // A B      R[A] = R[A](R[A+1] .. R[A+B]), the result replaces the callee
        Return,     // A B      returns R[A] if B != 0, nothing (nil) otherwise
        Closure,    // A Bx     R[A] = new function from Prototype.protos[Bx]

        Count
    };

    constexpr size_t MaxRegisters = 256;    // A, B, C valent 8 bits
    constexpr uint32_t MaxBx = 0xFFFF;
    constexpr int32_t MaxSBx = 0x7FFF;

    // Singification de l'operand, utilis� par le d�sassembleur
    enum class Operand : uint8_t { None, Reg, Const, Global, Imm, Upval, Proto, Jump };
    enum class OpFormat : uint8_t { ABC, ABx, AsBx };

    struct OpInfo
    {
        char const* name;
        OpFormat format;
        Operand a, b, c;
    };

    OpInfo const& GetOpInfo(OpCode _op);

    // Encoding
    Instruction EncodeABC(OpCode _op, uint8_t _a, uint8_t _b = 0, uint8_t _c = 0);
    Instruction EncodeABx(OpCode _op, uint8_t _a, uint16_t _bx);
    Instruction EncodeAsBx(OpCode _op, uint8_t _a, int32_t _sbx);

    // Decoding
    OpCode GetOp(Instruction _i);
    uint8_t GetA(Instruction _i);
    uint8_t GetB(Instruction _i);
    uint8_t GetC(Instruction _i);
    uint16_t GetBx(Instruction _i);
    int32_t GetSBx(Instruction _i);

    Instruction WithSBx(Instruction _i, int32_t _sbx);
}
#endif 