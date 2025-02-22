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
        BINOP_LESS_THAN_OR_EQUAL = 14,
        BINOP_GREATER_THAN_OR_EQUAL = 16,
        BINOP_NOT_EQUAL_TO = 18,
        BINOP_EQUAL_TO = 20
    };
    enum UnaryOpType {
        UNARY_NEGATE = 0
    };

    bool bin_op_is_bitwise_operator(BinOpType type);
    bool unary_is_bitwise_operator(UnaryOpType type);
    
    const char* bin_op_to_string(BinOpType type);
    const char* unary_op_to_string(UnaryOpType type);
};

#endif