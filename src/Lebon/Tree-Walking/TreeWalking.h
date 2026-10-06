#ifndef TREE_WALKING_H_INCLUDED
#define TREE_WALKING_H_INCLUDED

#include <functional>
#include <variant>
#include "Parser/AST.h"

namespace RUNTIME
{
    struct Callable;
    struct Environment;
    
    using Value = std::variant<std::monostate, float, bool, std::string, std::shared_ptr<Callable>>;
    
    struct RuntimeError
    {
        std::string message;
        uint32_t row, column;
    };
    
    struct Callable
    {
        std::string name;
        FuncDecl* decl = nullptr;                  
        std::shared_ptr<Environment> closure;          
        std::function<Value(std::vector<Value>&)> native;
    };
    
    inline std::string ToString(Value const& _v)
    {
        auto f = std::get_if<float>(&_v);
        auto b = std::get_if<bool>(&_v);
        auto s = std::get_if<std::string>(&_v);
        if (f)
            return std::to_string(*f);
        if (b)
        {
            if (*b)
                return "true";
            return "false";
        }
        if (s)
            return *s;
        
        return "-1"; 
    }
    
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
    
    class TreeWalking : Visitor
    {
        Value m_result;
        std::shared_ptr<Environment> m_env;
        bool m_returning = false;
        
        void DefineBuiltIns();
        Value Eval(Node& _n) { _n.Accept(*this); return std::move(m_result); }
        
    public:
        void Run(Program& _p);
        
        void Visit(NumberLiteral&)  override;
        void Visit(StringLiteral&)  override;
        void Visit(BooleanLiteral&) override;
        void Visit(Identifier&)     override;
        void Visit(UnaryExpr&)      override;
        void Visit(BinaryExpr&)     override;
        void Visit(AssignExpr&)     override;
        void Visit(CallExpr&)       override;
        void Visit(VarDecl&)        override;
        void Visit(ExprStmt&)       override;
        void Visit(ReturnStmt&)     override;
        void Visit(Block&)          override;
        void Visit(FuncDecl&)       override;
        void Visit(Program&)        override;
        
    };
}

#endif
