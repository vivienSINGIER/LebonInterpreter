#ifndef VALUE_HPP_INCLUDED
#define VALUE_HPP_INCLUDED

#include <string>
#include <variant>
#include <memory>
#include <functional>

#include "../Parser/AST.h"

namespace Runtime
{
    struct Callable;
    struct Environment;

    using Value = std::variant<std::monostate, float, bool, std::string,
                               std::shared_ptr<Callable>>;

    struct Callable
    {
        std::string name;
        FuncDecl* decl = nullptr;                       
        std::shared_ptr<Environment> closure;              
        std::function<Value(std::vector<Value>&)> native;  
    };

    struct RuntimeError
    {
        std::string message;
        uint32_t row, column;
    };
    
    struct Environment
    {
        std::unordered_map<std::string, Value> vars;
        std::shared_ptr<Environment> parent;

        explicit Environment(std::shared_ptr<Environment> _parent = nullptr)
            : parent(std::move(_parent)) {}

        void Define(std::string const& _name, Value _v) { vars[_name] = std::move(_v); }

        Value* Lookup(std::string const& _name)
        {
            for (Environment* e = this; e; e = e->parent.get())
            {
                auto it = e->vars.find(_name);
                if (it != e->vars.end())
                    return &it->second;
            }
            return nullptr;
        }
    };
}

#endif