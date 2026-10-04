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
        }
        return "unknown";
    }
}

namespace Semantics
{
    bool Analyser::Run(Program& _program)
    {
        _program.Accept(*this);
        
        return m_errorCount == 0;
    }

    void Analyser::DefineBuiltIns()
    {
        SymbolInfo affise;
        affise.name = "afise";
        affise.type = SymbolType::Function;
        affise.infType = InferredType::Unknown;
        affise.isBuiltIn = true;
        affise.isInitialized = true;
        affise.paramInfo = { { "value", InferredType::Unknown } };
        m_stack.Define("afise", affise);
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

    void Analyser::Visit(Program& _p)
    {
        m_stack.Push(ScopeType::Global);
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
            type = m_lastType;
        }
        
        bool isDefined = m_stack.Lookup(_varDecl.name) != nullptr;
        
        if (isDefined)
            Report(_varDecl.row, _varDecl.column, "duplicate variable declaration '" + _varDecl.name + "'");
        
        SymbolInfo s;
        s.name = _varDecl.name;
        s.infType = type;
        s.isBuiltIn = false;
        s.isInitialized = isInitialized;
        s.line = _varDecl.row;
        s.column = _varDecl.column;
        s.type = m_stack.scopes.back().type == ScopeType::Global ? SymbolType::GlobalVariable : SymbolType::LocalVariable;
        m_stack.Define(_varDecl.name, s);
    }

    void Analyser::Visit(Identifier& _identifier)
    {
        bool isDefined = m_stack.Lookup(_identifier.name) != nullptr;
        if (isDefined == false)
        {
            Report(_identifier.row, _identifier.column, "unknown identifier '" + _identifier.name + "' used");
            m_lastType = InferredType::Unknown;
            return;
        }
        
        SymbolInfo* s = m_stack.Lookup(_identifier.name);
        if (s->type == SymbolType::Function)
        {
            Report(_identifier.row, _identifier.column, "function '" + s->name + "' used as a value");
            m_lastType = InferredType::Unknown;
            return;
        }
        
        bool isInitialized = s->isInitialized;
        if (isInitialized == false)
            Report(_identifier.row, _identifier.column, "uninitialized identifier '" + _identifier.name + "' used");
        
        m_lastType = s->infType;
    }

    void Analyser::Visit(AssignExpr& _expr)
    {
        _expr.value->Accept(*this);
        SymbolInfo* s = m_stack.Lookup(_expr.name);
        
        if (s == nullptr)
        {
            Report(_expr.row, _expr.column, "unknown identifier '" + _expr.name + "' used");
            return;
        }
        
        if (s->isBuiltIn)
        {
            Report(_expr.row, _expr.column, "can't assign '" + _expr.name + "' to built-in '" + s->name + "'");
            return;
        }
        if (s->type == SymbolType::Function)
        {
            Report(_expr.row, _expr.column, "can't assign var '" + _expr.name + "' to function '" + s->name + "'");
            return;
        }
        if (s->infType != InferredType::Unknown && m_lastType != InferredType::Unknown && m_lastType != s->infType)
        {
            Report(_expr.row, _expr.column, "variable types don't match"); // TODO clean error
            return;
        }
        
        if (s->infType == InferredType::Unknown)
            s->infType = m_lastType;
        s->isInitialized = true;
    }

    void Analyser::Visit(Block& _b)
    {
        m_stack.Push(ScopeType::Block);
        for (auto& s : _b.statements)
            s->Accept(*this);
        m_stack.Pop();
    }

    void Analyser::Visit(NumberLiteral&)
    {
        m_lastType = InferredType::Number;
    }

    void Analyser::Visit(StringLiteral&)
    {
        m_lastType = InferredType::String;
    }

    void Analyser::Visit(BooleanLiteral&)
    {
        m_lastType = InferredType::Bool;
    }

    void Analyser::Visit(ExprStmt& _expr)
    {
        _expr.expr->Accept(*this);
    }

    void Analyser::Visit(FuncDecl& _func)
    {
        SymbolInfo* s = m_stack.Lookup(_func.name);
        
        if (s != nullptr)
        {
            Report(_func.row, _func.column, "function '" + _func.name + "' declaration already exists" );
            return;
        }
        
        SymbolInfo sym;
        sym.name = _func.name;
        for (auto& p : _func.params)
            sym.paramInfo.push_back(ParamInfo{p.name, InferredType::Unknown, p.row, p.column});
        sym.infType = InferredType::Unknown;
        sym.isInitialized = true;
        sym.isBuiltIn = false;
        sym.line = _func.row;
        sym.column = _func.column;
        sym.type = SymbolType::Function;
        
        m_stack.Define(_func.name, sym);
        m_stack.Push(ScopeType::FunctionBody, _func.name);
        
        std::vector<std::string_view> tempNames;
        for (auto& p : sym.paramInfo)
        {
            SymbolInfo pS;
            pS.name = p.name;
            pS.line = p.row;
            pS.column = p.column;
            pS.infType = InferredType::Unknown;
            pS.type = SymbolType::Param;
            pS.isInitialized = true;
            pS.isBuiltIn = false;
            
            bool exists = m_stack.LookupSingle(p.name);
            
            if (exists)
                Report(pS.line, pS.column, "duplicate parameter '" + pS.name + "'");
            m_stack.Define(pS.name, pS);
        }
        
        for (auto& st : _func.body->statements)
            st->Accept(*this);
        
        m_stack.Pop();
    }

    void Analyser::Visit(CallExpr& _call)
    {
        // Arguments are checked first so their errors show even when the call itself is wrong
        for (auto& arg : _call.args)
            arg->Accept(*this);

        m_lastType = InferredType::Unknown;

        Identifier* id = dynamic_cast<Identifier*>(_call.callee.get());
        if (id == nullptr)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            return;
        }

        SymbolInfo* s = m_stack.Lookup(id->name);
        if (s == nullptr)
        {
            Report(_call.row, _call.column, "function '" + id->name + "' doesn't exists" );
            return;
        }

        if (s->type != SymbolType::Function)
        {
            Report(_call.row, _call.column, "expression isn't callable");
            return;
        }

        if (s->paramInfo.size() != _call.args.size())
        {
            Report(_call.row, _call.column, "incorrect argument count for function '" + id->name + "' : expecter (" + std::to_string(s->paramInfo.size()) + "), got (" + std::to_string(_call.args.size()) + ")" );
            return;
        }

        m_lastType = s->infType;
    }

    void Analyser::Visit(ReturnStmt& _rtrn)
    {
        SymbolInfo* func = nullptr;
        
        for (auto it = m_stack.scopes.rbegin(); it != m_stack.scopes.rend(); ++it)
        {
            if (it->type != ScopeType::FunctionBody)
                continue;
            
            auto& symbols = std::next(it)->symbols;
            auto symIt = symbols.find(it->funcName);
            if (symIt != symbols.end())
                func = &symIt->second;
            break;
        }
        
        if (func == nullptr)
        {
            Report(_rtrn, "return expression used outside of a function");
            return;
        }
        
        if (_rtrn.value != nullptr)
            _rtrn.value->Accept(*this);
        else
            m_lastType = InferredType::Unknown;
        
        func->infType = m_lastType;
    }

    void Analyser::Visit(UnaryExpr& _expr)
    {
        _expr.operand->Accept(*this);
        
        if (m_lastType != InferredType::Number && m_lastType != InferredType::Unknown)
            Report(_expr, "operand must be a number");
        
        m_lastType = InferredType::Number;
    }

    void Analyser::Visit(BinaryExpr& _expr)
    {
        _expr.left->Accept(*this);
        InferredType lType = m_lastType;
        _expr.right->Accept(*this);
        InferredType rType = m_lastType;
        
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
                m_lastType = InferredType::Unknown;
                return;
            }

            m_lastType = lKnown && rKnown ? lType : InferredType::Unknown;
            return;
        }

        bool incorrect = lKnown && lType != InferredType::Number;
        incorrect = incorrect || (rKnown && rType != InferredType::Number);
        if (incorrect)
            Report(_expr, std::string("operands must be numbers, got ") + TypeName(lType) + " and " + TypeName(rType));

        m_lastType = InferredType::Number;
    }
}
