#include "OpCode.hpp"

namespace Bytecode
{
    OpInfo const& GetOpInfo(OpCode _op)
    {
        using O = Operand;
        using F = OpFormat;
        static constexpr OpInfo table[] = 
        {
            { "MOVE",      F::ABC,  O::Reg,  O::Reg,   O::None },
            { "LOADK",     F::ABx,  O::Reg,  O::Const, O::None },
            { "LOADBOOL",  F::ABC,  O::Reg,  O::Imm,   O::None },
            { "LOADNIL",   F::ABC,  O::Reg,  O::None,  O::None },
            { "GETGLOBAL", F::ABx,  O::Reg,  O::Global, O::None },
            { "SETGLOBAL", F::ABx,  O::Reg,  O::Global, O::None },
            { "GETUPVAL",  F::ABC,  O::Reg,  O::Upval, O::None },
            { "SETUPVAL",  F::ABC,  O::Reg,  O::Upval, O::None },
            { "ADD",       F::ABC,  O::Reg,  O::Reg,   O::Reg  },
            { "CONCAT",    F::ABC,  O::Reg,  O::Reg,   O::Reg  },
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
            { "ADDK",      F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "CONCATK",   F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "SUBK",      F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "MULK",      F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "DIVK",      F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "EQK",       F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "LTK",       F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "LEK",       F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "GTK",       F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "GEK",       F::ABC,  O::Reg,  O::Reg,   O::Const },
            { "JMP",      F::AsBx, O::None, O::Jump,  O::None },
            { "JMPIFNOT",  F::AsBx, O::Reg,  O::Jump,  O::None },
            { "CALL",      F::ABC,  O::Reg,  O::Imm,   O::None },
            { "RETURN",    F::ABC,  O::Reg,  O::Imm,   O::None },
            { "CLOSURE",   F::ABx,  O::Reg,  O::Proto, O::None },
        };

        static_assert(sizeof(table) / sizeof(table[0]) == static_cast<size_t>(OpCode::Count), "OpInfo table out of sync with OpCode");

        return table[static_cast<size_t>(_op)];
    }

    // Encoding
    Instruction EncodeABC(OpCode _op, uint8_t _a, uint8_t _b, uint8_t _c)
    {
        return static_cast<Instruction>(_op) | (Instruction(_a) << 8) | (Instruction(_b) << 16) | (Instruction(_c) << 24);
    }

    Instruction EncodeABx(OpCode _op, uint8_t _a, uint16_t _bx)
    {
        return static_cast<Instruction>(_op) | (Instruction(_a) << 8) | (Instruction(_bx) << 16);
    }

    Instruction EncodeAsBx(OpCode _op, uint8_t _a, int32_t _sbx)
    {
        return EncodeABx(_op, _a, static_cast<uint16_t>(_sbx + MaxSBx));
    }

    // Decoding
    OpCode GetOp(Instruction _i) { return static_cast<OpCode>(_i & 0xFF); }

    uint8_t GetA(Instruction _i) { return static_cast<uint8_t>((_i >> 8) & 0xFF); }
    uint8_t GetB(Instruction _i) { return static_cast<uint8_t>((_i >> 16) & 0xFF); }
    uint8_t GetC(Instruction _i) { return static_cast<uint8_t>((_i >> 24) & 0xFF); }
    uint16_t GetBx(Instruction _i) { return static_cast<uint16_t>(_i >> 16); }

    int32_t GetSBx(Instruction _i) { return static_cast<int32_t>(GetBx(_i)) - MaxSBx; }


    Instruction WithSBx(Instruction _i, int32_t _sbx)
    {
        return EncodeAsBx(GetOp(_i), GetA(_i), _sbx);
    }
}
