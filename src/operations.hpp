#ifndef _SGCPP_OPERATIONS_HPP
#define _SGCPP_OPERATIONS_HPP

#include "globals.hpp"

namespace Operations {
    enum BinOpType {
        BINOP_ADD,
        BINOP_SUB,
        BINOP_MUL,
        BINOP_DIV,
        BINOP_MOD,
        BINOP_LESS_THAN,
        BINOP_GREATER_THAN,
        BINOP_NOT_EQUAL_TO,
        BINOP_EQUAL_TO
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