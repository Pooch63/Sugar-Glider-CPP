#include "operations.hpp"

#ifdef DEBUG
#include <cassert>
#endif

bool Operations::bin_op_is_bitwise_operator([[maybe_unused]] BinOpType type) {
    return false;
};
bool Operations::unary_is_bitwise_operator([[maybe_unused]] UnaryOpType type) {
    return false;
}

const char* Operations::bin_op_to_string(BinOpType type) {
    switch (type) {
        case BinOpType::BINOP_ADD: return "+";
        case BinOpType::BINOP_SUB: return "-";
        case BinOpType::BINOP_MUL: return "*";
        case BinOpType::BINOP_DIV: return "/";
        case BinOpType::BINOP_MOD: return "%";
        case BinOpType::BINOP_LESS_THAN: return "<";
        case BinOpType::BINOP_GREATER_THAN: return ">";
        case BinOpType::BINOP_LESS_THAN_OR_EQUAL: return "<=";
        case BinOpType::BINOP_GREATER_THAN_OR_EQUAL: return ">=";
        case BinOpType::BINOP_NOT_EQUAL_TO: return "!=";
        case BinOpType::BINOP_EQUAL_TO: return "==";
        /* Otherwise, there was an error. Unknown binary operation type. */
        default:
            throw sg_assert_error("Tried to convert unknown BinOpType to string");
    }
};
const char* Operations::unary_op_to_string(UnaryOpType type) {
    switch (type) {
        case UnaryOpType::UNARY_NEGATE: return "-";
        /* Otherwise, there was an error. Unknown unary operation type. */
        default:
            throw sg_assert_error("Tried to convert unknown UnaryOpType to string");
    }
};