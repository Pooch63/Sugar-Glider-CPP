#include "value.hpp"

#include <bits/stdc++.h>
#include <limits>

namespace Math {
    #include <math.h>
}

#ifdef DEBUG
#include <cassert>
#endif

using namespace Values;

Value::Value(ValueType type, number_t number) : type(type), value(value_mem_t{ .number = number }) {};
Value::Value(ValueType type) : type(type) {};

bool Value::is_numerical() const {
    return this->type == ValueType::NUMBER ||
        this->type == ValueType::TRUE ||
        this->type == ValueType::FALSE;
};
Values::number_t Value::to_number() const {
    switch (this->type) {
        case ValueType::NUMBER: return this->value.number;
        case ValueType::TRUE: return static_cast<Values::number_t>(1);
        case ValueType::FALSE: return static_cast<Values::number_t>(0);
        default:
            #ifdef DEBUG
            /* Invalid numerical type */
            assert(false);
            #endif
    }
}

// Calculate a % b. Get the number you must subtract from "a" in order to make
// "a" a multiple of b.
static Values::number_t mod(Values::number_t a, Values::number_t b) {
    if (b == 0) {
        if ((a > 0) ^ (b > 0)) return -std::numeric_limits<double>::infinity();
        return std::numeric_limits<double>::infinity();
    }
    if (a < 0) return b - fmod(Math::fabs(a), b);

    return fmod(a, b);
}

Value* Values::bin_op(Operations::BinOpType type, Value a, Value b) {
    using Operations::BinOpType;

    if (!a.is_numerical() || !b.is_numerical()) {
        return nullptr;
    }

    Values::number_t first = a.to_number(),
        second = b.to_number();

    int firstI, secondI;

    if (Operations::bin_op_is_bitwise_operator(type)) {
        /* Make sure the numbers are both integers. */
        firstI = floor(first);
        secondI = floor(second);
    }

    switch (type) {
        case BinOpType::BINOP_ADD:
            return new Value(ValueType::NUMBER, first + second);
        case BinOpType::BINOP_SUB:
            return new Value(ValueType::NUMBER, first - second);
        case BinOpType::BINOP_MUL:
            return new Value(ValueType::NUMBER, first * second);
        case BinOpType::BINOP_DIV:
            return new Value(ValueType::NUMBER, first / second);
        case BinOpType::BINOP_MOD:
            return new Value(ValueType::NUMBER, mod(first, second));
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};
Value* Values::unary_op(Operations::UnaryOpType type, Value arg) {
    using Operations::UnaryOpType;

    if (!arg.is_numerical()) {
        return nullptr;
    }

    Values::number_t num = arg.to_number();

    int argI;

    if (Operations::unary_is_bitwise_operator(type)) {
        /* Make sure the numbers is an integer. */
        argI = floor(num);
    }

    switch (type) {
        case UnaryOpType::UNARY_NEGATE:
            return new Value(ValueType::NUMBER, -num);
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};