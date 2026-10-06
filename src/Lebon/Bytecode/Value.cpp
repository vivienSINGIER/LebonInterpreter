#include "Value.h"

#include <cstring>
#include <sstream>

namespace Bytecode
{
    bool operator==(Value const& _left, Value const& _right)
    {
        if (_left.type != _right.type)
            return false;

        switch (_left.type)
        {
        case ValueType::Nil:    return true;
        case ValueType::Bool:   return _left.b == _right.b;
        case ValueType::Number: return _left.n == _right.n;
        default:                return _left.o == _right.o;
        }
    }

    bool operator!=(Value const& _left, Value const& _right)
    {
        return !(_left == _right); 
    }

    bool SameConstant(Value const& _left, Value const& _right)
    {
        if (_left.type != _right.type)
            return false;

        if (_left.type == ValueType::Number)
            return std::memcmp(&_left.n, &_right.n, sizeof(float)) == 0;

        return _left == _right;
    }

    std::string ToString(Value const& _value)
    {
        switch (_value.type)
        {
        case ValueType::Nil:    return "nil";
        case ValueType::Bool:   return _value.b ? "true" : "false";
        case ValueType::Number:
        {
            std::ostringstream out;
            out << _value.n;
            return out.str();
        }
        case ValueType::String:   return _value.AsString()->chars;
        case ValueType::Function: return "<function>";
        case ValueType::Native:   return "<native>";
        }
        return "?";
    }

    std::string ToDebugString(Value const& _value)
    {
        if (_value.IsString())
            return "\"" + _value.AsString()->chars + "\"";
        return ToString(_value);
    }

    Value Value::MakeBool(bool _b)
    {
        Value value;
        value.type = ValueType::Bool;
        value.b = _b; 

        return value;
    }

    Value Value::MakeNumber(float _n)
    {
        Value value; 
        value.type = ValueType::Number; 
        value.n = _n; 

        return value;
    }

    Value Value::MakeObj(ValueType _type, Obj* _o)
    {
        Value value;
        value.type = _type;
        value.o = _o; 

        return value;
    }

    Value Value::MakeString(StringObj* _s)
    {
        return MakeObj(ValueType::String, _s);
    }
}
