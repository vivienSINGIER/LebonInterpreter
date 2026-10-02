#ifndef SYMBOLS_HPP_DEFINED
#define SYMBOLS_HPP_DEFINED
#include <string>
#include <unordered_map>
#include <vector>

namespace Semantics
{
    enum class SymbolType
    {
        Function, Param, LocalVariable, BuiltIn, GlobalVariable
    };
    
    enum class InferredType
    {
        Unknown, Bool, String, Number
    };
    
    enum class ScopeType
    {
        Global, FunctionBody, Block
    };
    
    struct ParamInfo
    {
        std::string name;
    };
    
    struct SymbolInfo
    {
        SymbolType type;
        std::string name;
        int line = 0;
        int column = 0;
        
        int arity = 0;
        std::vector<ParamInfo> paramInfo;
    };
    
    struct Scope
    {
        ScopeType type;
        std::unordered_map<std::string, SymbolInfo> symbols;
    };
    
    struct ScopeStack
    {
        std::vector<Scope> scopes;
        
        void Push(ScopeType _type)
        {
            scopes.push_back(Scope{_type, {}});
        }
        
        void Pop()
        {
            if (scopes.size() > 1)
                scopes.pop_back();
        }
        
        void Define(std::string const& _name, SymbolInfo _sInfo)
        {
            if (!scopes.empty())
            {
                scopes.back().symbols[_name] = _sInfo;
            }
        }
        
        SymbolInfo* Lookup(std::string const& _name)
        {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto symIt = it->symbols.find(_name);
                if (symIt != it->symbols.end())
                    return &symIt->second;
            }
            return nullptr;
        }
        
        SymbolInfo* LookupSingle(std::string const& _name)
        {
            if (scopes.empty() == false)
            {
                auto& symbols = scopes.back().symbols;
                auto symIt = symbols.find(_name);
                if (symIt != symbols.end())
                    return &symIt->second;
            }
            return nullptr;
        }
    };
}

#endif