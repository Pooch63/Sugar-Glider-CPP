#ifndef _SGCPP_OPERATIONS_HPP
#define _SGCPP_OPERATIONS_HPP

#include "globals.hpp"

namespace Operations {
    enum BinOpType {
        BINOP_ADD,
        BINOP_SUB,
        BINOP_MUL,
        BINOP_DIV,
        BINOP_MOD
    };
    enum UnaryOpType {
        UNARY_NEGATE
    };
    
    #ifdef DEBUG
    const char* bin_op_to_string(BinOpType type);
    const char* unary_op_to_string(UnaryOpType type);
    #endif
};

#endif