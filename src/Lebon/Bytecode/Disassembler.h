#ifndef BYTECODE_DISASSEMBLER_H_DEFINED
#define BYTECODE_DISASSEMBLER_H_DEFINED

#include <iostream>
#include <string>
#include <vector>

#include "Prototype.hpp"

namespace Bytecode
{
    // Prints a function and, after it, every function declared inside it.
    //
    //   == function addition (params=2, registers=3) ==
    //   0000  L4   ADD       R2 R0 R1
    //   0001  L4   RETURN    R2 1
    //
    // _globals : name of each global by slot (SymbolTable::GlobalNames), only used to comment GETGLOBAL / SETGLOBAL
    void Disassemble(Prototype const& _proto, std::ostream& _out = std::cout, std::vector<std::string> const& _globals = {});

    // One instruction on its own, without the index and the row
    std::string DisassembleInstruction(Prototype const& _proto, size_t _index, std::vector<std::string> const& _globals = {});
}

#endif
