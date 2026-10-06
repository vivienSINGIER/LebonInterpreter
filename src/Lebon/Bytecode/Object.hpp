#ifndef BYTECODE_OBJECT_HPP_DEFINED
#define BYTECODE_OBJECT_HPP_DEFINED

#include <cstdint>
#include <string>
#include <vector>

#include "Value.h"

namespace Bytecode
{
    struct Prototype;   // only pointed to here, Prototype.hpp includes Value.h and not this file
    class VM;

    enum class ObjType : uint8_t { String, Function, Native, Upvalue };

    // Everything a Value can point to. Owned by the Heap
    struct Obj
    {
        ObjType type;
        explicit Obj(ObjType _type) : type(_type) {}
        virtual ~Obj() = default;
    };

    struct StringObj : Obj
    {
        std::string chars;
        explicit StringObj(std::string _chars) : Obj(ObjType::String), chars(std::move(_chars)) {}
    };

    struct UpvalueObj : Obj
    {
        Value* location;
        Value closed;
        UpvalueObj* nextOpen = nullptr;     // open upvalues are chained, the highest stack slot first

        explicit UpvalueObj(Value* _slot) : Obj(ObjType::Upvalue), location(_slot) {}
    };

    // A function of the program : a prototype plus the variables it captured
    struct FunctionObj : Obj
    {
        Prototype const* proto;
        std::vector<UpvalueObj*> upvalues;

        explicit FunctionObj(Prototype const* _proto) : Obj(ObjType::Function), proto(_proto) {}
    };

    // A function written in C++ (afise)
    using NativeFn = Value(*)(VM& _vm, Value const* _args, int _count);

    struct NativeObj : Obj
    {
        std::string name;
        int arity;
        NativeFn fn;

        NativeObj(std::string _name, int _arity, NativeFn _fn)
            : Obj(ObjType::Native), name(std::move(_name)), arity(_arity), fn(_fn) {
        }
    };
}

#endif