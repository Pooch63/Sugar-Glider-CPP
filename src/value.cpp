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

Value::Value(ValueType type, std::vector<Value> *array) : type(type), value(value_mem_t{ .array = array }) {
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::ARRAY);
    #endif
}
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
Value::Value(Bytecode::constant_index_t prog_func_index, ValueType type) :
    type(type), value(value_mem_t{ .prog_func_index = prog_func_index })
{
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::PROGRAM_FUNCTION);
    #endif
};
Value::Value(native_method_t native) : type(ValueType::NATIVE_FUNCTION), value(value_mem_t{ .native = native }) {};
Value::Value(ValueType type) : type(type) {};
Value::Value() : type(ValueType::NULL_VALUE) {};

std::string Values::value_to_string(const Value &value) {
    switch (get_value_type(value)) {
        case ValueType::NUMBER: return std::to_string(get_value_number(value));
        case ValueType::TRUE:   return "true";
        case ValueType::FALSE:  return "false";
        case ValueType::STRING: return *get_value_string(value);
        case ValueType::NULL_VALUE: return "null";
        case ValueType::PROGRAM_FUNCTION: return "function";
        case ValueType::NATIVE_FUNCTION: return "[native function]";
        case ValueType::ARRAY: {
            std::string str = "[ ";
            bool found_value = false;
            for (Value value : *get_value_array(value)) {
                if (found_value) str += ", ";
                str += value_to_string(value);
                found_value = true;
            }
            str += " ]";
            return str;
        }
        default:
            throw sg_assert_error("Unknown value to log as string");
    }
};
std::string Values::value_to_debug_string(const Value &value) {
    switch (get_value_type(value)) {
        case ValueType::NULL_VALUE: return "null";
        case ValueType::NUMBER: return std::to_string(get_value_number(value));
        case ValueType::TRUE:   return "true";
        case ValueType::FALSE:  return "false";
        case ValueType::STRING: return '"' + *get_value_string(value) + '"';
        case ValueType::PROGRAM_FUNCTION: return "function";
        case ValueType::NATIVE_FUNCTION: return "[native function]";
        case ValueType::ARRAY: {
            std::string str = "[ ";
            bool found_value = false;
            for (Value value : *get_value_array(value)) {
                if (found_value) str += ", ";
                str += value_to_debug_string(value);
                found_value = true;
            }
            str += " ]";
            return str;
        }
        default:
            throw sg_assert_error("Unknown value to log as string");
    }
};

bool Values::value_is_truthy(const Value &value) {
    switch (get_value_type(value)) {
        case ValueType::NULL_VALUE: return false;
        case ValueType::TRUE: return true;
        case ValueType::FALSE: return false;
        case ValueType::NUMBER: return get_value_type(value) != 0;
        case ValueType::STRING: return get_value_string(value)->size() > 0;
        case ValueType::NATIVE_FUNCTION: return true;
        default:
            throw sg_assert_error("Unknown value to get truthy value from");
    }
};
bool Values::value_is_numerical(const Value &value) {
    return get_value_type(value) == ValueType::NUMBER ||
        get_value_type(value) == ValueType::TRUE ||
        get_value_type(value) == ValueType::FALSE;
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

bool Values::values_are_equal(const Value &a, const Value &b) {
    if (get_value_type(a) != get_value_type(b)) return false;
    switch (get_value_type(a)) {
        case ValueType::STRING: return *get_value_string(a) == *get_value_string(b);
        case ValueType::NATIVE_FUNCTION: return get_value_native_function(a).func == get_value_native_function(b).func;
        default: return true;
    }
};

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
        get_value_type(a) == ValueType::STRING &&
        get_value_type(b) == ValueType::STRING
    ) {
        *result = Value(
            ValueType::STRING,
            new std::string(*get_value_string(a) + *get_value_string(b)));
        return true;
    }
    if (type == BinOpType::BINOP_NOT_EQUAL_TO) {
        *result = Value(values_are_equal(a, b) ? ValueType::FALSE : ValueType::TRUE);
        return true;
    }

    if (!value_is_numerical(a) || !value_is_numerical(b)) {
        if (error != nullptr) {
            *error = "Cannot perform binary operation ";
            *error += Operations::bin_op_to_string(type);
            *error += " on values ";
            *error += value_to_string(a);
            *error += " and ";
            *error += value_to_string(b);
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
                *error += value_to_string(a);
                *error += " and ";
                *error += value_to_string(b);
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

    if (!value_is_numerical(arg)) {
        if (error != nullptr) {
            *error = "Cannot perform unary operation ";
            *error += Operations::unary_op_to_string(type);
            *error += " on value ";
            *error += value_to_string(arg);
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
                *error += value_to_string(arg);
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
        case ValueType::STRING: delete get_value_string(*this); break;
        case ValueType::ARRAY: delete get_value_array(*this); break;
        
        default: break;
    }
};