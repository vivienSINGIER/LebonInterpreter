#include "Disassembler.h"

#include <iomanip>
#include <sstream>

namespace Bytecode
{
    namespace
    {
        std::string Format(Operand _kind, int32_t _value, size_t _index)
        {
            switch (_kind)
            {
            case Operand::Reg:   return "R" + std::to_string(_value);
            case Operand::Const: return "K" + std::to_string(_value);
            case Operand::Global: return "G" + std::to_string(_value);
            case Operand::Upval: return "U" + std::to_string(_value);
            case Operand::Proto: return "P" + std::to_string(_value);
            case Operand::Imm:   return std::to_string(_value);
            case Operand::Jump:
            {
                std::ostringstream out;
                out << "-> " << std::setw(4) << std::setfill('0') << (static_cast<int32_t>(_index) + 1 + _value);
                return out.str();
            }
            case Operand::None:  break;
            }
            return "";
        }

        void Append(std::string& _text, std::string const& _operand)
        {
            if (_operand.empty())
                return;
            if (!_text.empty())
                _text += " ";
            _text += _operand;
        }
    }

    std::string DisassembleInstruction(Prototype const& _proto, size_t _index, std::vector<std::string> const& _globals)
    {
        Instruction i = _proto.code[_index];
        OpCode op = GetOp(i);
        if (op >= OpCode::Count)
            return "???";

        OpInfo const& info = GetOpInfo(op);

        std::string operands;
        Operand bKind = info.b;
        int32_t b = 0;
        if (info.format == OpFormat::ABC)
            b = GetB(i);
        else if (info.format == OpFormat::ABx)
            b = GetBx(i);
        else
            b = GetSBx(i);

        Append(operands, Format(info.a, GetA(i), _index));
        Append(operands, Format(bKind, b, _index));
        if (info.format == OpFormat::ABC)
            Append(operands, Format(info.c, GetC(i), _index));

        std::ostringstream out;
        out << std::left << std::setw(9) << info.name << ' ' << operands;

        // Show what the constant actually is
        if (bKind == Operand::Const && static_cast<size_t>(b) < _proto.constants.size())
            out << "  ; " << ToDebugString(_proto.constants[b]);
        else if (bKind == Operand::Global && static_cast<size_t>(b) < _globals.size())
            out << "  ; " << _globals[b];
        else if (bKind == Operand::Proto && static_cast<size_t>(b) < _proto.protos.size())
            out << "  ; " << _proto.protos[b]->name;

        return out.str();
    }

    void Disassemble(Prototype const& _proto, std::ostream& _out, std::vector<std::string> const& _globals)
    {
        _out << "== function " << (_proto.name.empty() ? "<main>" : _proto.name)
             << " (params=" << static_cast<int>(_proto.numParams)
             << ", registers=" << _proto.maxRegisters << ") ==\n";

        for (size_t i = 0; i < _proto.code.size(); ++i)
        {
            _out << std::setw(4) << std::setfill('0') << i << std::setfill(' ')
                 << "  L" << std::left << std::setw(3) << _proto.rows[i] << std::right
                 << " " << DisassembleInstruction(_proto, i, _globals) << "\n";
        }

        for (auto const& child : _proto.protos)
        {
            _out << "\n";
            Disassemble(*child, _out, _globals);
        }
    }
}
