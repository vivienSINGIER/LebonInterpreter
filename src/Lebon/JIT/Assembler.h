#ifndef JIT_ASSEMBLER_H_DEFINED
#define JIT_ASSEMBLER_H_DEFINED

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Jit
{
    // Writes x64 machine code in a buffer, one method per instruction form.
    // Knows nothing about Lebon, the code generator decides what to emit
    class Assembler
    {
    public:
        std::vector<uint8_t> const& Code() const { return m_code; }
        size_t Size() const { return m_code.size(); }
        
        void Patch32(size_t _pos, uint32_t _value);
        
        // FUNCTION FRAME
        void PushRbp();
        void MovRbpRsp();
        void SubRspImm32(uint32_t _value); 
        void MovRspRbp();
        void PopRbp();
        void Ret();
        
        // CONSTANTS
        void MovEaxImm32(uint32_t _value);
        void MovRaxImm64(uint64_t _value);
        void MovdXmm0Eax();
        
        // FRAME SLOTS
        void MovRaxRbp(int32_t _offset);
        void MovRbpRax(int32_t _offset);
        void MovssXmm0Rbp(int32_t _offset);
        void MovssRbpXmm0(int32_t _offset);
        
        // GLOBALS
        void MovRcxImm64(uint64_t _address);
        void MovRaxArcx();
        void MovArcxRax();
        void MovssXmm0Arcx();
        void MovssArcxXmm0();
        
        // ARITHMETIC
        void MovapsX1X0();
        void AddsX0X1();
        void SubssX0X1();
        void Mulss();
        void Divss();
        void Xorps();
        
        // CALLS
        void MovRspRax(uint32_t _offset);
        void MovssRspX0(uint32_t _offset);
        void Callr32(size_t _target);
        void MovRcxRax();
        void MovRdxRax();
        void MovRcxRbp(int32_t _offset);
        void CallRax();

    private:
        std::vector<uint8_t> m_code;

        // Multi-byte values are written little endian, the way x64 reads them
        void Emit8(uint8_t _byte);
        void Emit32(uint32_t _value);
        void Emit64(uint64_t _value);
    };
}

#endif
