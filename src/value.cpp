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

Value::Value(ValueType type, std::string* str) : type(type), value(value_mem_t{ .str = str }) {
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::STRING);
    #endif
}
Value::Value(ValueType type, number_t number) : type(type), value(value_mem_t{ .number = number }) {
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::NUMBER);
    #endif
};
Value::Value(native_method_t native) : type(ValueType::NATIVE_FUNCTION), value(value_mem_t{ .native = native }) {};
Value::Value(ValueType type) : type(type) {};
Value::Value() : type(ValueType::NULL_VALUE) {};

std::string Value::to_string() const {
    switch (this->get_type()) {
        case ValueType::NUMBER: return std::to_string(this->get_number());
        case ValueType::TRUE:   return "true";
        case ValueType::FALSE:  return "false";
        case ValueType::STRING: return *this->get_string();
        case ValueType::NULL_VALUE: return "null";
        case ValueType::NATIVE_FUNCTION: return "[native function]";
        default:
            throw sg_assert_error("Unknown value to log as string");
    }
};
std::string Value::to_debug_string() const {
    switch (this->get_type()) {
        case ValueType::NULL_VALUE: return "null";
        case ValueType::NUMBER: return std::to_string(this->get_number());
        case ValueType::TRUE:   return "true";
        case ValueType::FALSE:  return "false";
        case ValueType::STRING: return '"' + *this->get_string() + '"';
        case ValueType::NATIVE_FUNCTION: return "[native function]";
        default:
            throw sg_assert_error("Unknown value to log as string");
    }
};

bool Value::is_truthy() const {
    switch (this->get_type()) {
        case ValueType::NULL_VALUE: return false;
        case ValueType::TRUE: return true;
        case ValueType::FALSE: return false;
        case ValueType::NUMBER: return this->get_number() != 0;
        case ValueType::STRING: return this->get_string()->size() > 0;
        case ValueType::NATIVE_FUNCTION: return true;
        default:
            throw sg_assert_error("Unknown value to get truthy value from");
    }
};
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
            throw sg_assert_error("Tried to convert non-numeric value type to number");
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

bool Values::bin_op(Operations::BinOpType type, Value a, Value b, Value *result, std::string *error) {
    using Operations::BinOpType;

    if (
        type == BinOpType::BINOP_ADD &&
        a.get_type() == ValueType::STRING &&
        b.get_type() == ValueType::STRING) {
            *result = Value(
                ValueType::STRING,
                new std::string(*a.get_string() + *b.get_string()));
                return true;
        }

    if (!a.is_numerical() || !b.is_numerical()) {
        if (error != nullptr) {
            *error = "Cannot perform binary operation ";
            *error += Operations::bin_op_to_string(type);
            *error += " on values ";
            *error += a.to_debug_string();
            *error += " and ";
            *error += b.to_debug_string();
        }
        return false;
    }

    Values::number_t first = a.to_number(),
        second = b.to_number();

    int firstI, secondI;

    if (Operations::bin_op_is_bitwise_operator(type)) {
        /* Make sure the numbers are both integers. */
        firstI = floor(first);
        secondI = floor(second);

        /* Bitwise operation error on floats */
        if (first != firstI && second != secondI) {
            if (error != nullptr) {
                *error = "Cannot perform bitwise operation ";
                *error += Operations::bin_op_to_string(type);
                *error += " on non-integer values ";
                *error += a.to_debug_string();
                *error += " and ";
                *error += b.to_debug_string();
            }
            return false;
        }
    }

    switch (type) {
        case BinOpType::BINOP_ADD:
            *result = Value(ValueType::NUMBER, first + second); break;
        case BinOpType::BINOP_SUB:
            *result = Value(ValueType::NUMBER, first - second); break;
        case BinOpType::BINOP_MUL:
            *result = Value(ValueType::NUMBER, first * second); break;
        case BinOpType::BINOP_DIV:
            *result = Value(ValueType::NUMBER, first / second); break;
        case BinOpType::BINOP_MOD:
            *result = Value(ValueType::NUMBER, mod(first, second)); break;
        case BinOpType::BINOP_LESS_THAN:
            *result = Value(first < second ? ValueType::TRUE : ValueType::FALSE); break;
        case BinOpType::BINOP_GREATER_THAN:
            *result = Value(first > second ? ValueType::TRUE : ValueType::FALSE); break;
        default:
            throw sg_assert_error("Tried to compute unknown binary operation on two values");
    }

    return true;
};
bool Values::unary_op(Operations::UnaryOpType type, Value arg, Value *result, std::string *error) {
    using Operations::UnaryOpType;

    if (!arg.is_numerical()) {
        if (error != nullptr) {
            *error = "Cannot perform unary operation ";
            *error += Operations::unary_op_to_string(type);
            *error += " on value ";
            *error += arg.to_debug_string();
        }
        return false;
    }

    Values::number_t num = arg.to_number();

    int argI;

    if (Operations::unary_is_bitwise_operator(type)) {
        /* Make sure the numbers is an integer. */
        argI = floor(num);

        /* Bitwise operation error on floats */
        if (num != argI) {
            if (error != nullptr) {
                *error = "Cannot perform bitwise operation ";
                *error += Operations::unary_op_to_string(type);
                *error += " on non-integer value ";
                *error += arg.to_debug_string();
            }
            return false;
        }
    }

    switch (type) {
        case UnaryOpType::UNARY_NEGATE:
            *result = Value(ValueType::NUMBER, -num); break;
        default:
            throw sg_assert_error("Tried to compute unknown unary operation on value");
    }

    return true;
};

void Value::free_payload() {
    if (!this->should_free_payload) return;
    switch (this->type) {
        case ValueType::STRING: delete this->get_string(); break;
        default: break;
    }
};