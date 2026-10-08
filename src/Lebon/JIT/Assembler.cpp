#include "Assembler.h"

namespace Jit
{
    // FUNCTION FRAME

    void Assembler::Patch32(size_t _pos, uint32_t _value)
    {
        for (int i = 0; i < 4; i++)
            m_code[_pos + i] = static_cast<uint8_t>(_value >> (i * 8));
    }

    // push rbp                 55                      prologue
    void Assembler::PushRbp()
    {
        Emit8(0x55);
    }

    // mov rbp, rsp             48 89 E5                prologue
    void Assembler::MovRbpRsp()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0xE5);
    }

    // sub rsp, i32             48 81 EC i32            prologue, reserves the slots
    void Assembler::SubRspImm32(uint32_t _value)
    {
        Emit8(0x48);
        Emit8(0x81);
        Emit8(0xEC);
        Emit32(_value);
    }

    // mov rsp, rbp             48 89 EC                epilogue
    void Assembler::MovRspRbp()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0xEC);
    }

    // pop rbp                  5D                      epilogue
    void Assembler::PopRbp()
    {
        Emit8(0x5D);
    }

    // ret                      C3                      epilogue
    void Assembler::Ret()
    {
        Emit8(0xC3);
    }

    // CONSTANTS

    // mov eax, i32             B8 i32                  bool literals, float bits
    void Assembler::MovEaxImm32(uint32_t _value)
    {
        Emit8(0xB8);
        Emit32(_value);
    }

    // mov rax, i64             48 B8 i64               string pointers, helper addresses
    void Assembler::MovRaxImm64(uint64_t _value)
    {
        Emit8(0x48);
        Emit8(0xB8);
        Emit64(_value);
    }

    // movd xmm0, eax           66 0F 6E C0             turns float bits into a number
    void Assembler::MovdXmm0Eax()
    {
        Emit8(0x66);
        Emit8(0x0F);
        Emit8(0x6E);
        Emit8(0xC0);
    }

    // FRAME SLOTS (params, locals, temporaries)
    // The offset is in bytes from rbp : negative for locals and temporaries, positive for params

    // mov rax, [rbp+d32]       48 8B 85 d32            loads a string or a bool
    void Assembler::MovRaxRbp(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x8B);
        Emit8(0x85);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // mov [rbp+d32], rax       48 89 85 d32            stores a string or a bool
    void Assembler::MovRbpRax(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0x85);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // movss xmm0, [rbp+d32]    F3 0F 10 85 d32         loads a number
    void Assembler::MovssXmm0Rbp(int32_t _offset)
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x10);
        Emit8(0x85);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // movss [rbp+d32], xmm0    F3 0F 11 85 d32         stores a number
    void Assembler::MovssRbpXmm0(int32_t _offset)
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x11);
        Emit8(0x85);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // GLOBALS

    // mov rcx, i64             48 B9 i64               address of the global's slot
    void Assembler::MovRcxImm64(uint64_t _address)
    {
        Emit8(0x48);
        Emit8(0xB9);
        Emit64(_address);
    }

    // mov rax, [rcx]           48 8B 01                loads a string or a bool
    void Assembler::MovRaxArcx()
    {
        Emit8(0x48);
        Emit8(0x8B);
        Emit8(0x01);
    }

    // mov [rcx], rax           48 89 01                stores a string or a bool
    void Assembler::MovArcxRax()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0x01);
    }

    // movss xmm0, [rcx]        F3 0F 10 01             loads a number
    void Assembler::MovssXmm0Arcx()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x10);
        Emit8(0x01);
    }

    // movss [rcx], xmm0        F3 0F 11 01             stores a number
    void Assembler::MovssArcxXmm0()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x11);
        Emit8(0x01);
    }

    // ARITHMETIC

    // movaps xmm1, xmm0        0F 28 C8                moves the right operand aside
    void Assembler::MovapsX1X0()
    {
        Emit8(0x0F);
        Emit8(0x28);
        Emit8(0xC8);
    }

    // addss xmm0, xmm1         F3 0F 58 C1             add on numbers
    void Assembler::Adds()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x58);
        Emit8(0xC1);
    }

    // subss xmm0, xmm1         F3 0F 5C C1             sub
    void Assembler::Subss()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x5C);
        Emit8(0xC1);
    }

    // mulss xmm0, xmm1         F3 0F 59 C1             mul
    void Assembler::Mulss()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x59);
        Emit8(0xC1);
    }

    // divss xmm0, xmm1         F3 0F 5E C1             div
    void Assembler::Divss()
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x5E);
        Emit8(0xC1);
    }

    // xorps xmm0, xmm0         0F 57 C0                zero, so unary minus is 0 - x
    void Assembler::Xorps()
    {
        Emit8(0x0F);
        Emit8(0x57);
        Emit8(0xC0);
    }

    // CALLS

    // mov [rsp+d32], rax       48 89 84 24 d32         passes a string or a bool to a Lebon function
    void Assembler::MovRspRax(uint32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0x84);
        Emit8(0x24);
        Emit32(_offset);
    }

    // movss [rsp+d32], xmm0    F3 0F 11 84 24 d32      passes a number to a Lebon function
    void Assembler::MovssRspX0(uint32_t _offset)
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x11);
        Emit8(0x84);
        Emit8(0x24);
        Emit32(_offset);
    }

    // call rel32               E8 d32                  calls a Lebon function
    // _target is the offset of the function in this buffer. What is written is the distance
    // from the end of the call to that function, so the code works wherever it is loaded
    void Assembler::Callr32(size_t _target)
    {
        Emit8(0xE8);
        int64_t end = static_cast<int64_t>(m_code.size()) + 4;
        Emit32(static_cast<uint32_t>(static_cast<int64_t>(_target) - end));
    }

    // mov rcx, rax             48 89 C1                1st argument of a C++ helper
    void Assembler::MovRcxRax()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0xC1);
    }

    // mov rdx, rax             48 89 C2                2nd argument of a C++ helper
    void Assembler::MovRdxRax()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0xC2);
    }

    // mov rcx, [rbp+d32]       48 8B 8D d32            1st argument of a C++ helper, taken from a temp slot
    void Assembler::MovRcxRbp(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x8B);
        Emit8(0x8D);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // call rax                 FF D0                   calls a C++ helper
    void Assembler::CallRax()
    {
        Emit8(0xFF);
        Emit8(0xD0);
    }

    // PARENT FRAMES
    // A nested function reaches the variables of its parents through rcx, which holds the rbp of the frame they live in

    // mov rcx, rbp             48 89 E9                starts from the frame of the current function
    void Assembler::MovRcxRbpReg()
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0xE9);
    }

    // mov rcx, [rcx+d32]       48 8B 89 d32            goes one frame up, d32 is where the parent link is
    void Assembler::MovRcxArcxOff(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x8B);
        Emit8(0x89);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // mov [rsp+d32], rcx       48 89 8C 24 d32         passes the parent link to a Lebon function
    void Assembler::MovRspRcx(uint32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0x8C);
        Emit8(0x24);
        Emit32(_offset);
    }

    // movss xmm0, [rcx+d32]    F3 0F 10 81 d32         loads a number of a parent
    void Assembler::MovssXmm0ArcxOff(int32_t _offset)
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x10);
        Emit8(0x81);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // movss [rcx+d32], xmm0    F3 0F 11 81 d32         stores a number of a parent
    void Assembler::MovssArcxOffXmm0(int32_t _offset)
    {
        Emit8(0xF3);
        Emit8(0x0F);
        Emit8(0x11);
        Emit8(0x81);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // mov rax, [rcx+d32]       48 8B 81 d32            loads a string or a bool of a parent
    void Assembler::MovRaxArcxOff(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x8B);
        Emit8(0x81);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // mov [rcx+d32], rax       48 89 81 d32            stores a string or a bool of a parent
    void Assembler::MovArcxOffRax(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x89);
        Emit8(0x81);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // CONDITIONS
    // A comparison writes its result in the flags of the processor, a set instruction turns them into 0 or 1

    // ucomiss xmm0, xmm1       0F 2E C1                compares two numbers
    void Assembler::CmpX0X1()
    {
        Emit8(0x0F);
        Emit8(0x2E);
        Emit8(0xC1);
    }

    // cmp rax, [rbp+d32]       48 3B 85 d32            compares two strings or two bools, one of them in a slot
    void Assembler::CmpRaxRbp(int32_t _offset)
    {
        Emit8(0x48);
        Emit8(0x3B);
        Emit8(0x85);
        Emit32(static_cast<uint32_t>(_offset));
    }

    // sete al                  0F 94 C0                1 if equal
    void Assembler::SeteAl()
    {
        Emit8(0x0F);
        Emit8(0x94);
        Emit8(0xC0);
    }

    // setne al                 0F 95 C0                1 if not equal
    void Assembler::SetneAl()
    {
        Emit8(0x0F);
        Emit8(0x95);
        Emit8(0xC0);
    }

    // setb al                  0F 92 C0                1 if lower
    void Assembler::SetlAl()
    {
        Emit8(0x0F);
        Emit8(0x92);
        Emit8(0xC0);
    }

    // seta al                  0F 97 C0                1 if greater
    void Assembler::SetgAl()
    {
        Emit8(0x0F);
        Emit8(0x97);
        Emit8(0xC0);
    }

    // setbe al                 0F 96 C0                1 if lower or equal
    void Assembler::SetleAl()
    {
        Emit8(0x0F);
        Emit8(0x96);
        Emit8(0xC0);
    }

    // setae al                 0F 93 C0                1 if greater or equal
    void Assembler::SetgeAl()
    {
        Emit8(0x0F);
        Emit8(0x93);
        Emit8(0xC0);
    }

    // movzx eax, al            0F B6 C0                clears the rest of rax, only al was written by the set
    void Assembler::MovzxEaxAl()
    {
        Emit8(0x0F);
        Emit8(0xB6);
        Emit8(0xC0);
    }

    // test eax, eax            85 C0                   checks a condition before a jump
    void Assembler::TestEaxEax()
    {
        Emit8(0x85);
        Emit8(0xC0);
    }

    // jz rel32                 0F 84 d32               jumps if the condition is false
    // The distance is counted from the end of the jump. It is emitted as 0 when the target comes later,
    // then written with Patch32 at Size() - 4
    void Assembler::JzRel32(int32_t _distance)
    {
        Emit8(0x0F);
        Emit8(0x84);
        Emit32(static_cast<uint32_t>(_distance));
    }

    // jmp rel32                E9 d32                  always jumps, the distance works like the one of jz
    void Assembler::JmpRel32(int32_t _distance)
    {
        Emit8(0xE9);
        Emit32(static_cast<uint32_t>(_distance));
    }

    void Assembler::Emit8(uint8_t _byte)
    {
        m_code.push_back(_byte);
    }

    void Assembler::Emit32(uint32_t _value)
    {
        for (int i = 0; i < 4; i++)
            Emit8(static_cast<uint8_t>(_value >> (8 * i)));
    }

    void Assembler::Emit64(uint64_t _value)
    {
        for (int i = 0; i < 8; i++)
            Emit8(static_cast<uint8_t>(_value >> (8 * i)));
    }
}
