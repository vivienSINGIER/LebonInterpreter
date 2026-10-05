#include "Analyser.h"

#include <vector>

namespace
{
    char const* TypeName(Semantics::InferredType _type)
    {
        switch (_type)
        {
        case Semantics::InferredType::Unknown: return "unknown";
        case Semantics::InferredType::Bool:    return "bool";
        case Semantics::InferredType::String:  return "string";
        case Semantics::InferredType::Number:  return "number";
        case Semantics::InferredType::Void:    return "void";
        case Semantics::InferredType::Error:   return "error";
        case Semantics::InferredType::Any:     return "any";
        }
        return "unknown";
    }

    bool IsKnown(Semantics::InferredType _type)
    {
        return _type != Semantics::InferredType::Unknown && _type != Semantics::InferredType::Error;
    }

    void Infer(Semantics::InferredType& _slot, Semantics::InferredType _type)
    {
        if (_slot == Semantics::InferredType::Unknown)
            _slot = _type;
    }
}

namespace Semantics
{

    bool Analyser::Run(Program& _program)
    {
        m_stack = &_program.stack;
        _program.Accept(*this);
        
        for (Expr* e : m_exprs) e->type = m_types.Get(e->typeVar);
        for (SymbolInfo& s : m_stack->table.symbols)
            if (s.typeVar != InvalidTypeVar) 
                s.infType = m_types.Get(s.typeVar);
        
        return m_errorCount == 0;
    }

    void Analyser::DefineBuiltIns()
    {
        SymbolInfo affiseP;
        affiseP.name = "_out";
        affiseP.type = SymbolType::Param;
        affiseP.typeVar = m_types.New(InferredType::Any);
        affiseP.isBuiltIn = true;
        affiseP.isInitialized = false;
        SymbolId id = m_stack->table.Create(affiseP);
        
        SymbolInfo affise;
        affise.name = "afise";
        affise.type = SymbolType::Function;
        affise.typeVar = m_types.New(InferredType::Void);
        affise.isBuiltIn = true;
        affise.isInitialized = true;
        affise.params = { id };
        m_stack->Define("afise", affise);
    }

    void Analyser::Report(Node const& _at, std::string const& _message)
    {
        Report(_at.row, _at.column, _message);
    }

    void Analyser::Report(uint32_t _row, uint32_t _column, std::string const& _message)
    {
        ErrorManager::LogError(Error::Semantics(_message, _row, _column));
        m_errorCount++;
    }

    InferredType Analyser::ValueType(Expr const& _expr)
    {
        InferredType type = m_types.Get(_expr.typeVar);
        if (type != InferredType::Void)
            return type;
        
        Report(_expr, "expression has no value");
        return InferredType::Error;
    }

    TypeVar Analyser::ValueVar(Expr const& _expr)
    {
        if (m_types.Get(_expr.typeVar) != InferredType::Void)
            return _expr.typeVar;

        Report(_expr, "expression has no value");
        return m_types.New(InferredType::Error);
    }

    void Analyser::SetVar(Expr& _expr, Semantics::TypeVar _var)
    {
        _expr.typeVar = _var;
        m_exprs.push_back(&_expr);
    }

    void Analyser::Visit(Program& _p)
    {
        m_stack->Push(ScopeType::Global);
        DefineBuiltIns();
        
        for (auto& s : _p.statements)
            s->Accept(*this);
        
        m_stack->Pop();
    }

    void Analyser::Visit(VarDecl& _varDecl)
    {
        bool isInitialized = false;
        TypeVar typeVar = m_types.New();
        if (_varDecl.init != nullptr)
        {
            _varDecl.init->Accept(*this);
            isInitialized = true;
            typeVar = ValueVar(*_varDecl.init);
        }
        
        bool isDefined = m_stack->Lookup(_varDecl.name) != InvalidSymbolId;
        
        if (isDefined)
            Report(_varDecl.row, _varDecl.column, "duplicate variable declaration '" + _varDecl.name + "'");
        
        SymbolInfo s;
        s.name = _varDecl.name;
        s.typeVar = typeVar;
        s.isBuiltIn = false;
        s.isInitialized = isInitialized;
        s.line = _varDecl.row;
        s.column = _varDecl.column;
        s.type = m_stack->scopes.back().type == ScopeType::Global ? SymbolType::GlobalVariable : SymbolType::LocalVariable;
        _varDecl.symbol = m_stack->Define(_varDecl.name, s);
    }

    void Analyser::Visit(Identifier& _identifier)
    {
        SymbolId id = m_stack->Lookup(_identifier.name);
        if (id == InvalidSymbolId)
        {
            Report(_identifier.row, _identifier.column, "unknown identifier '" + _identifier.name + "' used");
            _identifier.type = InferredType::Error;
            return;
        }
        _identifier.symbol = id;

        SymbolInfo& s = m_stack->table.Get(id);
        if (s.type == SymbolType::Function)
        {
            Report(_identifier.row, _identifier.column, "function '" + s.name + "' used as a value");
            _identifier.type = InferredType::Error;
            return;
        }

        if (s.isInitialized == false)
        {
            Report(_identifier.row, _identifier.column, "uninitialized identifier '" + _identifier.name + "' used");
            _identifier.type = InferredType::Error;
            return;
        }

        SetVar(_identifier, s.typeVar);
    }

    void Analyser::Visit(AssignExpr& _expr)
    {
        _expr.value->Accept(*this);
        InferredType t = ValueType(*_expr.value);
        SymbolId id = m_stack->Lookup(_expr.name);

        if (id == InvalidSymbolId)
        {
            Report(_expr.row, _expr.column, "unknown identifier '" + _expr.name + "' used");
            _expr.type = InferredType::Error;
            return;
        }
        _expr.symbol = id;

        SymbolInfo& s = m_stack->table.Get(id);
        if (s.isBuiltIn)
        {
            Report(_expr.row, _expr.column, "can't assign '" + _expr.name + "' to built-in '" + s.name + "'");
            _expr.type = InferredType::Error;
            return;
        }
        if (s.type == SymbolType::Function)
        {
            Report(_expr.row, _expr.column, "can't assign var '" + _expr.name + "' to function '" + s.name + "'");
            _expr.type = InferredType::Error;
            return;
        }
        if (m_types.Unify(s.typeVar, _expr.value->typeVar) == false)
        {
            Report(_expr.row, _expr.column, "variable types don't match"); // TODO clean error
            _expr.type = InferredType::Error;
            return;
        }

        _expr.typeVar = s.typeVar;
        s.isInitialized = true;
    }

    void Analyser::Visit(Block& _b)
    {
        m_stack->Push(ScopeType::Block);
        for (auto& s : _b.statements)
            s->Accept(*this);
        m_stack->Pop();
    }

    void Analyser::Visit(NumberLiteral& _l)
    {
        SetVar(_l, m_types.New(InferredType::Number));
    }

    void Analyser::Visit(StringLiteral& _l)
    {
        SetVar(_l, m_types.New(InferredType::String));
    }

    void Analyser::Visit(BooleanLiteral& _l)
    {
         SetVar(_l, m_types.New(InferredType::Bool));
    }

    void Analyser::Visit(ExprStmt& _expr)
    {
        _expr.expr->Accept(*this);
    }

    void Analyser::Visit(FuncDecl& _func)
    {
        if (m_stack->Lookup(_func.name) != InvalidSymbolId)
            Report(_func.row, _func.column, "function '" + _func.name + "' declaration already exists" );
        
        SymbolInfo sym;
        sym.name = _func.name;
        sym.typeVar = m_types.New();
        sym.isInitialized = true;
        sym.isBuiltIn = false;
        sym.line = _func.row;
        sym.column = _func.column;
        sym.type = SymbolType::Function;
        
        SymbolId funcId = m_stack->Define(_func.name, sym);
        _func.symbol = funcId;
        m_stack->Push(ScopeType::FunctionBody, _func.name);
        
        for (auto& p : _func.params)
        {
            SymbolInfo pS;
            pS.name = p.name;
            pS.line = p.row;
            pS.column = p.column;
            pS.typeVar = m_types.New();
            pS.type = SymbolType::Param;
            pS.isInitialized = true;
            pS.isBuiltIn = false;
            
            bool exists = m_stack->LookupSingle(p.name) != InvalidSymbolId;
            
            if (exists)
                Report(pS.line, pS.column, "duplicate parameter '" + pS.name + "'");
            p.symbol = m_stack->Define(pS.name, pS);
            m_stack->table.Get(funcId).params.push_back(p.symbol);
        }
        
        for (auto& st : _func.body->statements)
            st->Accept(*this);
        
        bool hasReturn = m_stack->scopes.back().hasReturn;
        m_stack->Pop();
        
        if (hasReturn == false)
            m_types.Bind(m_stack->table.Get(funcId).typeVar, InferredType::Void);
    }

    void Analyser::Visit(CallExpr& _call)
    {
        std::vector<InferredType> argTypes;
        argTypes.reserve(_call.args.size());
        for (auto& arg : _call.args)
        {
            arg->Accept(*this);
            argTypes.push_back(ValueType(*arg));
        }

        Identifier* id = dynamic_cast<Identifier*>(_call.callee.get());
        if (id == nullptr)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            _call.type = InferredType::Error;
            return;
        }
        
        SymbolId funcId = m_stack->Lookup(id->name);
        if (funcId == InvalidSymbolId)
        {
            Report(_call.row, _call.column, "function '" + id->name + "' doesn't exists" );
            _call.type = InferredType::Error;
            id->type = InferredType::Error;
            return;
        }

        SymbolInfo& s = m_stack->table.Get(funcId);
        if (s.type != SymbolType::Function)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            _call.type = InferredType::Error;
            id->type = InferredType::Error;
            return;
        }
        _call.symbol = funcId;
        id->symbol = funcId;
        id->type = s.infType;

        if (s.params.size() != _call.args.size())
        {
            Report(_call.row, _call.column, "incorrect argument count for function '" + id->name + "' : expecter (" + std::to_string(s.params.size()) + "), got (" + std::to_string(_call.args.size()) + ")" );
            _call.type = InferredType::Error;
            return;
        }

        for (size_t i = 0; i < s.params.size(); i++)
        {
            SymbolInfo& sym = m_stack->table.Get(s.params[i]);
            InferredType aType = m_types.Get(_call.args[i]->typeVar);
            
            if (m_types.Unify(sym.typeVar, _call.args[i]->typeVar) == false)
                Report(*_call.args[i].get(), std::string("argument type doesn't match function signature, expected '") + TypeName(sym.infType) + "', got '" + TypeName(aType) + "'" );
        }
        
        SetVar(_call, s.typeVar);
    }

    void Analyser::Visit(ReturnStmt& _rtrn)
    {
        SymbolId funcId = InvalidSymbolId;

        for (auto it = m_stack->scopes.rbegin(); it != m_stack->scopes.rend(); ++it)
        {
            if (it->type != ScopeType::FunctionBody)
                continue;

            auto& symbols = std::next(it)->symbols;
            auto symIt = symbols.find(it->funcName);
            if (symIt != symbols.end())
                funcId = symIt->second;

            it->hasReturn = true;
            break;
        }

        if (funcId == InvalidSymbolId)
        {
            Report(_rtrn, "return expression used outside of a function");
            return;
        }
        
        if (_rtrn.value != nullptr)
            _rtrn.value->Accept(*this);
        
        TypeVar var = _rtrn.value != nullptr ? ValueVar(*_rtrn.value) : m_types.New(InferredType::Void);

        SymbolInfo& func = m_stack->table.Get(funcId);
        InferredType funcType = m_types.Get(func.typeVar);
        InferredType type = m_types.Get(var);
        if (m_types.Unify(func.typeVar, var) == false)
            Report(_rtrn, std::string("function returns both ") + TypeName(funcType) + " and " + TypeName(type));
    }

    void Analyser::Visit(UnaryExpr& _expr)
    {
        _expr.operand->Accept(*this);
        InferredType t = ValueType(*_expr.operand);
        
        if (IsKnown(t) && t != InferredType::Number)
            Report(_expr, "operand must be a number");
        _expr.type = InferredType::Number;
        Infer(_expr.operand->type, InferredType::Number);
    }

    void Analyser::Visit(BinaryExpr& _expr)
    {
        _expr.left->Accept(*this);
        InferredType lType = ValueType(*_expr.left);
        _expr.right->Accept(*this);
        InferredType rType = ValueType(*_expr.right);
        
        bool lKnown = IsKnown(lType);
        bool rKnown = IsKnown(rType);
        bool poisoned = lType == InferredType::Error || rType == InferredType::Error;

        if (_expr.op == TokenType::ADD)
        {
            if (poisoned)
            {
                _expr.type = InferredType::Error;
                return;
            }

            bool incorrect = lType == InferredType::Bool || rType == InferredType::Bool;
            incorrect = incorrect || m_types.Unify(_expr.left->typeVar, _expr.right->typeVar) == false;

            if (incorrect)
            {
                Report(_expr, std::string("can't add ") + TypeName(lType) + " and " + TypeName(rType));
                _expr.type = InferredType::Error;
                return;
            }
            
            _expr.typeVar = _expr.left->typeVar;
            return;
        }

        bool incorrect = !m_types.Bind(_expr.left->typeVar, InferredType::Number) || !m_types.Bind(_expr.right->typeVar, InferredType::Number);
        if (incorrect && poisoned == false)
            Report(_expr, std::string("operands must be numbers, got ") + TypeName(lType) + " and " + TypeName(rType));
        _expr.typeVar = m_types.New(InferredType::Number);
    }
}
