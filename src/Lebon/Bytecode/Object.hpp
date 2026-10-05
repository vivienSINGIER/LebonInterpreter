#ifndef BYTECODE_OBJECT_HPP_DEFINED
#define BYTECODE_OBJECT_HPP_DEFINED

#include <cstdint>
#include <string>

namespace Bytecode
{
    enum class ObjType : uint8_t { String, Function, Native };

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
}

#endif
