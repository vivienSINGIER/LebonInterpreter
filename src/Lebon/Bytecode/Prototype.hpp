#ifndef BYTECODE_PROTOTYPE_HPP_DEFINED
#define BYTECODE_PROTOTYPE_HPP_DEFINED

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "OpCode.hpp"
#include "Value.h"

namespace Bytecode
{
    struct UpvalDesc
    {
        bool fromParentRegister = false;
        uint8_t index = 0;
    };

    struct Prototype
    {
        std::string name;
        uint8_t numParams = 0;
        uint8_t maxRegisters = 0;     // registers the frame needs, up to MaxRegisters

        std::vector<Instruction> code;
        std::vector<uint32_t> rows;    // source row of each instruction, same size as code
        std::vector<Value> constants;
        std::vector<std::unique_ptr<Prototype>> protos;   // functions declared inside
        std::vector<UpvalDesc> upvalues;

        // Appends an instruction, returns its index (needed to patch jumps)
        size_t Emit(Instruction _i, uint32_t _row = 0)
        {
            code.push_back(_i);
            rows.push_back(_row);
            return code.size() - 1;
        }

        // Index of the next instruction to be emitted
        size_t Here() const { return code.size(); }

        // Points the jump at _jumpIndex to _target
        void PatchJump(size_t _jumpIndex, size_t _target)
        {
            int32_t offset = static_cast<int32_t>(_target) - static_cast<int32_t>(_jumpIndex) - 1;
            code[_jumpIndex] = WithSBx(code[_jumpIndex], offset);
        }

        // Index of the constant, reusing an identical one. -1 when the pool is full (MaxBx)
        int32_t AddConstant(Value const& _v)
        {
            for (size_t i = 0; i < constants.size(); ++i)
            {
                if (SameConstant(constants[i], _v))
                    return static_cast<int32_t>(i);
            }

            if (constants.size() > MaxBx)
                return -1;

            constants.push_back(_v);
            return static_cast<int32_t>(constants.size() - 1);
        }

        // Takes ownership of a nested function, returns its index for CLOSURE. -1 when full
        int32_t AddProto(std::unique_ptr<Prototype> _p)
        {
            if (protos.size() > MaxBx)
                return -1;

            protos.push_back(std::move(_p));
            return static_cast<int32_t>(protos.size() - 1);
        }
    };
}

#endif
