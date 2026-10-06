#ifndef JIT_OPP_CODES_HPP
#define JIT_OPP_CODES_HPP

#include <cstdint>

namespace jit
{
    enum OppCodes : uint8_t
    {
        LOADK, LOADBOOL, MOVE
        GETGLOBAL, SETGLOBAL,
        ADD_NUM, SUB_NUM, MUL_NUM, DIV_NUM, NEG_NUM,
        CONCAT_STR,
        CALL, RET, RET_VOID,
        PRINT_NUM, PRINT_STR, PRINT_BOOL, 
        LT, LE, GT, GE, EQ, EQ_STR, EQ_BOOL, 
        JUMP, JUMP_IF, JUMP_IF_NOT, 
        MOVE_STR, RELEASE
    };
    
    
}

#endif