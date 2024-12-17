#include "operations.hpp"

#ifdef DEBUG
#include <cassert>
#endif

#ifdef DEBUG
const char* Operations::bin_op_to_string(BinOpType type) {
    switch (type) {
        case BinOpType::BINOP_ADD: return "+";
        case BinOpType::BINOP_SUB: return "-";
        case BinOpType::BINOP_MUL: return "*";
        case BinOpType::BINOP_DIV: return "/";
        case BinOpType::BINOP_MOD: return "%";
        /* Otherwise, there was an error. Unknown binary operation type. */
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};
const char* Operations::unary_op_to_string(UnaryOpType type) {
    switch (type) {
        case UnaryOpType::UNARY_NEGATE: return "-";
        /* Otherwise, there was an error. Unknown unary operation type. */
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};
#endif