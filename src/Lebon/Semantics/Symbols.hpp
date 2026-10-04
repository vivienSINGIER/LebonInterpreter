#ifndef SYMBOLS_HPP_DEFINED
#define SYMBOLS_HPP_DEFINED
#include <string>
#include <unordered_map>
#include <vector>

namespace Semantics
{
    enum class SymbolType
    {
        Function, Param, GlobalVariable, LocalVariable
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
        InferredType type;
        uint32_t row;
        uint32_t column;
    };
    
    struct SymbolInfo
    {
        SymbolType type;
        InferredType infType;
        std::string name;
        
        uint32_t line = 0;
        uint32_t column = 0;
        
        bool isInitialized = false;
        bool isBuiltIn = false;
        
        std::vector<ParamInfo> paramInfo;
    };
    
    struct Scope
    {
        ScopeType type;
        std::unordered_map<std::string, SymbolInfo> symbols;
        std::string funcName;
    };
    
    struct ScopeStack
    {
        std::vector<Scope> scopes;
        
        void Push(ScopeType _type, std::string const& _funcName = "")
        {
            scopes.push_back(Scope{_type, {}, _funcName});
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