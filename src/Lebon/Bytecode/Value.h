#ifndef BYTECODE_VALUE_HPP_DEFINED
#define BYTECODE_VALUE_HPP_DEFINED

#include <cstdint>
#include <string>

namespace Bytecode
{
    // Only pointed to here : Object.hpp includes this file, so it cannot be included back
    struct Obj;
    struct StringObj;

    enum class ValueType : uint8_t { Nil, Bool, Number, String, Function, Native };

    struct Value
    {
        ValueType type = ValueType::Nil;
        union
        {
            bool b;
            float n;
            Obj* o;
        };

        Value() : n(0.0f) {}

        static Value MakeNil() { return Value(); }
        static Value MakeBool(bool _b);
        static Value MakeNumber(float _n);
        static Value MakeObj(ValueType _type, Obj* _o);
        static Value MakeString(StringObj* _s);

        bool IsNil() const { return type == ValueType::Nil; }
        bool IsBool() const { return type == ValueType::Bool; }
        bool IsNumber() const { return type == ValueType::Number; }
        bool IsString() const { return type == ValueType::String; }

        StringObj* AsString() const;    // defined in Value.cpp, it needs the full StringObj

        bool IsTruthy() const { return !(IsNil() || (IsBool() && !b)); }
    };

    bool operator==(Value const& _left, Value const& _right);
    bool operator!=(Value const& _left, Value const& _right);

    bool SameConstant(Value const& _left, Value const& _right);

    std::string ToString(Value const& _value);

    std::string ToDebugString(Value const& _value);
}

#endif