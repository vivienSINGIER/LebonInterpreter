#ifndef SYMBOLS_HPP_DEFINED
#define SYMBOLS_HPP_DEFINED
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Semantics
{
    enum class SymbolType
    {
        Function, Param, GlobalVariable, LocalVariable
    };
    
    enum class InferredType
    {
        Unknown, Bool, String, Number, Void
    };
    
    enum class ScopeType
    {
        Global, FunctionBody, Block
    };
    
    using SymbolId = uint32_t;
    constexpr SymbolId InvalidSymbolId = UINT32_MAX;
    
    struct SymbolInfo
    {
        SymbolType type;
        InferredType infType;
        std::string name;
        
        uint32_t line = 0;
        uint32_t column = 0;
        
        bool isInitialized = false;
        bool isBuiltIn = false;
        
        std::vector<SymbolId> params;
    };

    struct SymbolTable
    {
        std::vector<SymbolInfo> symbols;
        
        SymbolId Create(SymbolInfo _s)
        {
            symbols.push_back(std::move(_s));
            return static_cast<SymbolId>(symbols.size() - 1);
        }
        
        SymbolInfo& Get(SymbolId _id) { return symbols[_id]; }
    };
    
    struct Scope
    {
        ScopeType type;
        std::unordered_map<std::string, SymbolId> symbols;
        std::string funcName;
        bool hasReturn = false;
    };
    
    struct ScopeStack
    {
        std::vector<Scope> scopes;
        SymbolTable table;
        
        void Push(ScopeType _type, std::string const& _funcName = "")
        {
            scopes.push_back(Scope{_type, {}, _funcName, false});
        }
        
        void Pop()
        {
            if (scopes.size() > 1)
                scopes.pop_back();
        }
        
        SymbolId Define(std::string const& _name, SymbolInfo _sInfo)
        {
            if (scopes.empty())
                return InvalidSymbolId;

            SymbolId id = table.Create(std::move(_sInfo));
            scopes.back().symbols[_name] = id;
            return id;
        }
        
        SymbolId Lookup(std::string const& _name)
        {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto symIt = it->symbols.find(_name);
                if (symIt != it->symbols.end())
                    return symIt->second;
            }
            return InvalidSymbolId;
        }
        
        SymbolId LookupSingle(std::string const& _name)
        {
            if (scopes.empty() == false)
            {
                auto& symbols = scopes.back().symbols;
                auto symIt = symbols.find(_name);
                if (symIt != symbols.end())
                    return symIt->second;
            }
            return InvalidSymbolId;
        }
    };
}

#endif