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

        GetGlobal,  // A Bx     R[A] = Globals[K[Bx]]
        SetGlobal,  // A Bx     Globals[K[Bx]] = R[A]
        GetUpval,   // A B      R[A] = Upvalues[B]
        SetUpval,   // A B      Upvalues[B] = R[A]

        Add,        // A B C    R[A] = R[B] + R[C]  (numbers added, strings joined)
        Sub,        // A B C    R[A] = R[B] - R[C]
        Mul,        // A B C    R[A] = R[B] * R[C]
        Div,        // A B C    R[A] = R[B] / R[C]
        Neg,        // A B      R[A] = -R[B]
        Not,        // A B      R[A] = not R[B]

        Eq,         // A B C    R[A] = (R[B] == R[C])
        Lt,         // A B C    R[A] = (R[B] <  R[C])
        Le,         // A B C    R[A] = (R[B] <= R[C])
        Gt,         // A B C    R[A] = (R[B] >  R[C])
        Ge,         // A B C    R[A] = (R[B] >= R[C])

        Jmp,        // sBx      pc += sBx
        JmpIfNot,   // A sBx    if R[A] is false (or nil) then pc += sBx

        Call,       // A B      R[A] = R[A](R[A+1] .. R[A+B]), the result replaces the callee
        Return,     // A B      returns R[A] if B != 0, nothing (nil) otherwise
        Closure,    // A Bx     R[A] = new function from Prototype.protos[Bx]

        Count
    };

    constexpr size_t MaxRegisters = 256;    // A, B, C valent 8 bits
    constexpr uint32_t MaxBx = 0xFFFF;
    constexpr int32_t MaxSBx = 0x7FFF;

    // What an operand means, used by the disassembler and later by a verifier
    enum class Operand : uint8_t { None, Reg, Const, Imm, Upval, Proto, Jump };
    enum class OpFormat : uint8_t { ABC, ABx, AsBx };

    struct OpInfo
    {
        char const* name;
        OpFormat format;
        Operand a, b, c;
    };

    inline OpInfo const& GetOpInfo(OpCode _op)
    {
        using O = Operand;
        using F = OpFormat;
        static constexpr OpInfo table[] = 
        {
            { "MOVE",      F::ABC,  O::Reg,  O::Reg,   O::None },
            { "LOADK",     F::ABx,  O::Reg,  O::Const, O::None },
            { "LOADBOOL",  F::ABC,  O::Reg,  O::Imm,   O::None },
            { "LOADNIL",   F::ABC,  O::Reg,  O::None,  O::None },
            { "GETGLOBAL", F::ABx,  O::Reg,  O::Const, O::None },
            { "SETGLOBAL", F::ABx,  O::Reg,  O::Const, O::None },
            { "GETUPVAL",  F::ABC,  O::Reg,  O::Upval, O::None },
            { "SETUPVAL",  F::ABC,  O::Reg,  O::Upval, O::None },
            { "ADD",       F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "SUB",       F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "MUL",       F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "DIV",       F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "NEG",       F::ABC,  O::Reg,  O::Reg,   O::None },
            { "NOT",       F::ABC,  O::Reg,  O::Reg,   O::None },
            { "EQ",        F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "LT",        F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "LE",        F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "GT",        F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "GE",        F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "JMP",       F::AsBx, O::None, O::Jump,  O::None },
            { "JMPIFNOT",  F::AsBx, O::Reg,  O::Jump,  O::None },
            { "CALL",      F::ABC,  O::Reg,  O::Imm,   O::None },
            { "RETURN",    F::ABC,  O::Reg,  O::Imm,   O::None },
            { "CLOSURE",   F::ABx,  O::Reg,  O::Proto, O::None },
        };

        static_assert(sizeof(table) / sizeof(table[0]) == static_cast<size_t>(OpCode::Count), "OpInfo table out of sync with OpCode");

        return table[static_cast<size_t>(_op)];
    }

    // Encoding
    inline Instruction EncodeABC(OpCode _op, uint8_t _a, uint8_t _b = 0, uint8_t _c = 0)
    {
        return static_cast<Instruction>(_op) | (Instruction(_a) << 8) | (Instruction(_b) << 16) | (Instruction(_c) << 24);
    }

    inline Instruction EncodeABx(OpCode _op, uint8_t _a, uint16_t _bx)
    {
        return static_cast<Instruction>(_op) | (Instruction(_a) << 8) | (Instruction(_bx) << 16);
    }

    inline Instruction EncodeAsBx(OpCode _op, uint8_t _a, int32_t _sbx)
    {
        return EncodeABx(_op, _a, static_cast<uint16_t>(_sbx + MaxSBx));
    }

    // Decoding
    inline OpCode GetOp(Instruction _i) { return static_cast<OpCode>(_i & 0xFF); }

    inline uint8_t GetA(Instruction _i) { return static_cast<uint8_t>((_i >> 8) & 0xFF); }
    inline uint8_t GetB(Instruction _i) { return static_cast<uint8_t>((_i >> 16) & 0xFF); }
    inline uint8_t GetC(Instruction _i) { return static_cast<uint8_t>((_i >> 24) & 0xFF); }
    inline uint16_t GetBx(Instruction _i) { return static_cast<uint16_t>(_i >> 16); }

    inline int32_t GetSBx(Instruction _i) { return static_cast<int32_t>(GetBx(_i)) - MaxSBx; }


    inline Instruction WithSBx(Instruction _i, int32_t _sbx)
    {
        return EncodeAsBx(GetOp(_i), GetA(_i), _sbx);
    }
}
#endif 