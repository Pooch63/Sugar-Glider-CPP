#ifndef _SGCPP_OPERATIONS_HPP
#define _SGCPP_OPERATIONS_HPP

#include "globals.hpp"

namespace Operations {
    // Bitwise operators are odd, everything else is even
    enum BinOpType {
        BINOP_ADD = 0,
        BINOP_SUB = 2,
        BINOP_MUL = 4,
        BINOP_DIV = 6,
        BINOP_MOD = 8,
        BINOP_LESS_THAN = 10,
        BINOP_GREATER_THAN = 12,
        BINOP_NOT_EQUAL_TO = 14,
        BINOP_EQUAL_TO = 16
    };
    enum UnaryOpType {
        UNARY_NEGATE
    };

    bool bin_op_is_bitwise_operator(BinOpType type);
    bool unary_is_bitwise_operator(UnaryOpType type);
    
    const char* bin_op_to_string(BinOpType type);
    const char* unary_op_to_string(UnaryOpType type);
};

#endif