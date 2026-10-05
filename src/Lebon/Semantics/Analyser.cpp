#include "Analyser.h"

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
        }
        return "unknown";
    }
}

namespace Semantics
{

    bool Analyser::Run(Program& _program)
    {
        m_stack = &_program.stack;
        _program.Accept(*this);
        
        return m_errorCount == 0;
    }

    void Analyser::DefineBuiltIns()
    {
        SymbolInfo affiseP;
        affiseP.name = "_out";
        affiseP.type = SymbolType::Param;
        affiseP.infType = InferredType::Unknown;
        affiseP.isBuiltIn = true;
        affiseP.isInitialized = false;
        SymbolId id = m_stack->table.Create(affiseP);
        
        SymbolInfo affise;
        affise.name = "afise";
        affise.type = SymbolType::Function;
        affise.infType = InferredType::Void;
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
        if (_expr.type != InferredType::Void)
            return _expr.type;
        
        Report(_expr, "expression has no value");
        return InferredType::Unknown;
    }

    void Analyser::Visit(Program& _p)
    {
        m_stack->Push(ScopeType::Global);
        DefineBuiltIns();
        
        for (auto& s : _p.statements)
            s->Accept(*this);
    }

    void Analyser::Visit(VarDecl& _varDecl)
    {
        bool isInitialized = false;
        InferredType type = InferredType::Unknown;
        if (_varDecl.init != nullptr)
        {
            _varDecl.init->Accept(*this);
            isInitialized = true;
            type = ValueType(*_varDecl.init);
        }
        
        bool isDefined = m_stack->Lookup(_varDecl.name) != InvalidSymbolId;
        
        if (isDefined)
            Report(_varDecl.row, _varDecl.column, "duplicate variable declaration '" + _varDecl.name + "'");
        
        SymbolInfo s;
        s.name = _varDecl.name;
        s.infType = type;
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
            return;
        }
        _identifier.symbol = id;

        SymbolInfo& s = m_stack->table.Get(id);
        if (s.type == SymbolType::Function)
        {
            Report(_identifier.row, _identifier.column, "function '" + s.name + "' used as a value");
            return;
        }

        bool isInitialized = s.isInitialized;
        if (isInitialized == false)
            Report(_identifier.row, _identifier.column, "uninitialized identifier '" + _identifier.name + "' used");

        _identifier.type = s.infType;
    }

    void Analyser::Visit(AssignExpr& _expr)
    {
        _expr.value->Accept(*this);
        InferredType t = ValueType(*_expr.value);
        SymbolId id = m_stack->Lookup(_expr.name);

        if (id == InvalidSymbolId)
        {
            Report(_expr.row, _expr.column, "unknown identifier '" + _expr.name + "' used");
            return;
        }
        _expr.symbol = id;

        SymbolInfo& s = m_stack->table.Get(id);
        if (s.isBuiltIn)
        {
            Report(_expr.row, _expr.column, "can't assign '" + _expr.name + "' to built-in '" + s.name + "'");
            return;
        }
        if (s.type == SymbolType::Function)
        {
            Report(_expr.row, _expr.column, "can't assign var '" + _expr.name + "' to function '" + s.name + "'");
            return;
        }
        if (s.infType != InferredType::Unknown && t != InferredType::Unknown && t != s.infType)
        {
            Report(_expr.row, _expr.column, "variable types don't match"); // TODO clean error
            return;
        }

        if (s.infType == InferredType::Unknown)
            s.infType = t;
        _expr.type = t;
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
        _l.type = InferredType::Number;
    }

    void Analyser::Visit(StringLiteral& _l)
    {
         _l.type = InferredType::String;
    }

    void Analyser::Visit(BooleanLiteral& _l)
    {
         _l.type = InferredType::Bool;
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
        sym.infType = InferredType::Unknown;
        sym.isInitialized = true;
        sym.isBuiltIn = false;
        sym.line = _func.row;
        sym.column = _func.column;
        sym.type = SymbolType::Function;
        
        // Kept as an id, the table grows while the params and the body are visited
        SymbolId funcId = m_stack->Define(_func.name, sym);
        _func.symbol = funcId;
        m_stack->Push(ScopeType::FunctionBody, _func.name);
        
        for (auto& p : _func.params)
        {
            SymbolInfo pS;
            pS.name = p.name;
            pS.line = p.row;
            pS.column = p.column;
            pS.infType = InferredType::Unknown;
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
            m_stack->table.Get(funcId).infType = InferredType::Void;
    }

    void Analyser::Visit(CallExpr& _call)
    {
        for (auto& arg : _call.args)
        {
            arg->Accept(*this);
            ValueType(*arg);
        }

        Identifier* id = dynamic_cast<Identifier*>(_call.callee.get());
        if (id == nullptr)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            return;
        }

        SymbolId funcId = m_stack->Lookup(id->name);
        if (funcId == InvalidSymbolId)
        {
            Report(_call.row, _call.column, "function '" + id->name + "' doesn't exists" );
            return;
        }

        SymbolInfo& s = m_stack->table.Get(funcId);
        if (s.type != SymbolType::Function)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            return;
        }
        _call.symbol = funcId;
        id->symbol = funcId;

        if (s.params.size() != _call.args.size())
        {
            Report(_call.row, _call.column, "incorrect argument count for function '" + id->name + "' : expecter (" + std::to_string(s.params.size()) + "), got (" + std::to_string(_call.args.size()) + ")" );
            return;
        }

        _call.type = s.infType;
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

        InferredType type = InferredType::Void;
        if (_rtrn.value != nullptr)
        {
            _rtrn.value->Accept(*this);
            type = _rtrn.value->type;
        }

        SymbolInfo& func = m_stack->table.Get(funcId);
        bool known = type != InferredType::Unknown && func.infType != InferredType::Unknown;
        if (known && type != func.infType)
            Report(_rtrn, std::string("function returns both ") + TypeName(func.infType) + " and " + TypeName(type));
        else if (type != InferredType::Unknown)
            func.infType = type;
    }

    void Analyser::Visit(UnaryExpr& _expr)
    {
        _expr.operand->Accept(*this);
        InferredType t = ValueType(*_expr.operand);
        
        if (t != InferredType::Number && t != InferredType::Unknown)
            Report(_expr, "operand must be a number");
        _expr.type = InferredType::Number;
    }

    void Analyser::Visit(BinaryExpr& _expr)
    {
        _expr.left->Accept(*this);
        InferredType lType = ValueType(*_expr.left);
        _expr.right->Accept(*this);
        InferredType rType = ValueType(*_expr.right);
        
        bool lKnown = lType != InferredType::Unknown;
        bool rKnown = rType != InferredType::Unknown;

        if (_expr.op == TokenType::ADD)
        {
            // number + number or string + string, nothing else
            bool incorrect = lType == InferredType::Bool || rType == InferredType::Bool;
            incorrect = incorrect || (lKnown && rKnown && lType != rType);

            if (incorrect)
            {
                Report(_expr, std::string("can't add ") + TypeName(lType) + " and " + TypeName(rType));
                return;
            }

            _expr.type = lKnown && rKnown ? lType : InferredType::Unknown;
            return;
        }

        bool incorrect = lKnown && lType != InferredType::Number;
        incorrect = incorrect || (rKnown && rType != InferredType::Number);
        if (incorrect)
            Report(_expr, std::string("operands must be numbers, got ") + TypeName(lType) + " and " + TypeName(rType));
        _expr.type = InferredType::Number;
    }
}
