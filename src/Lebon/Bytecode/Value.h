#ifndef BYTECODE_VALUE_HPP_DEFINED
#define BYTECODE_VALUE_HPP_DEFINED

#include <cstdint>
#include <string>

#include "Object.hpp"

namespace Bytecode
{
    enum class ValueType : uint8_t { Nil, Bool, Number, String, Function, Native };

    struct Value
    {
        ValueType type = ValueType::Nil;
        union
        {
            bool b;
            double n;
            Obj* o;
        };

        Value() : n(0.0) {}

        static Value MakeNil() { return Value(); }
        static Value MakeBool(bool _b);
        static Value MakeNumber(double _n);
        static Value MakeObj(ValueType _type, Obj* _o);
        static Value MakeString(StringObj* _s);

        bool IsNil() const { return type == ValueType::Nil; }
        bool IsBool() const { return type == ValueType::Bool; }
        bool IsNumber() const { return type == ValueType::Number; }
        bool IsString() const { return type == ValueType::String; }

        StringObj* AsString() const { return static_cast<StringObj*>(o); }

        bool IsTruthy() const { return !(IsNil() || (IsBool() && !b)); }
    };

    bool operator==(Value const& _left, Value const& _right);
    bool operator!=(Value const& _left, Value const& _right);

    bool SameConstant(Value const& _left, Value const& _right);

    std::string ToString(Value const& _value);

    std::string ToDebugString(Value const& _value);
}

#endif
