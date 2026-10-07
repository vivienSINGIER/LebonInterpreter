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
        Unknown, Bool, String, Number, Void, Error, Any
    };
    
    enum class ScopeType
    {
        Global, FunctionBody, Block
    };
    
    using SymbolId = uint32_t;
    constexpr SymbolId InvalidSymbolId = UINT32_MAX;
    using TypeVar = uint32_t;
    constexpr TypeVar InvalidTypeVar = UINT32_MAX;
    constexpr uint32_t InvalidGlobalSlot = UINT32_MAX;

    struct TypeTable
    {
        std::vector<TypeVar> parents;
        std::vector<InferredType> types;
        
        
        TypeVar New(InferredType _type = InferredType::Unknown)
        {
            parents.push_back(static_cast<TypeVar>(parents.size()));
            types.push_back(_type);
            return static_cast<TypeVar>(parents.size() - 1);
        }

        TypeVar Find(TypeVar _v)
        {
            while (parents[_v] != _v)
            {
                parents[_v] = parents[parents[_v]];
                _v = parents[_v];
            }
            return _v;
        }

        InferredType Get(TypeVar _v) { return types[Find(_v)]; }
        
        static bool Absorbs(InferredType _t) { return _t == InferredType::Error || _t == InferredType::Any; }

        bool Bind(TypeVar _v, InferredType _type)
        {
            TypeVar root = Find(_v);
            if (types[root] == InferredType::Unknown) { types[root] = _type; return true; }
            return types[root] == _type || Absorbs(types[root]) || Absorbs(_type);
        }
        
        bool Unify(TypeVar _a, TypeVar _b)
        {
            TypeVar a = Find(_a), b = Find(_b);
            if (a == b || Absorbs(types[a]) || Absorbs(types[b])) return true;
            if (types[a] == InferredType::Unknown)      parents[a] = b;
            else if (types[b] == InferredType::Unknown) parents[b] = a;
            else return types[a] == types[b];
            return true;
        }
    };
    
    struct SymbolInfo
    {
        SymbolType type;
        InferredType infType = InferredType::Unknown;
        TypeVar typeVar = InvalidTypeVar;
        std::string name;
        
        uint32_t line = 0;
        uint32_t column = 0;
        
        bool isInitialized = false;
        bool isBuiltIn = false;

        // Index in the globals array of the VM, InvalidGlobalSlot if the symbol isn't a global
        uint32_t globalSlot = InvalidGlobalSlot;

        std::vector<SymbolId> params;
    };

    struct SymbolTable
    {
        std::vector<SymbolInfo> symbols;
        uint32_t globalCount = 0;

        SymbolId Create(SymbolInfo _s)
        {
            symbols.push_back(std::move(_s));
            return static_cast<SymbolId>(symbols.size() - 1);
        }
        
        SymbolInfo& Get(SymbolId _id) { return symbols[_id]; }

        // Name of each global, indexed by slot
        std::vector<std::string> GlobalNames() const
        {
            std::vector<std::string> names(globalCount);
            for (SymbolInfo const& s : symbols)
            {
                if (s.globalSlot != InvalidGlobalSlot)
                    names[s.globalSlot] = s.name;
            }
            return names;
        }
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

            if (scopes.back().type == ScopeType::Global)
                _sInfo.globalSlot = table.globalCount++;

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