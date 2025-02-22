#include "math.hpp"
#include "natives.hpp"

#include <math.h>

#include "array.hpp"
#include "natives.hpp"

using namespace Values;

// Possible check ranges
enum CheckMode {
    // All numbers
    REAL,
    // Input must be greater than 0
    LOGARITHM
};
// Checks if a value is numerical, and if it is, sets the given number argument to the value
static bool check_number(const char *func, std::string &error_message, Value value, Values::number_t &num, CheckMode mode) {
    if (value_is_numerical(value)) {
        num = value_to_number(value);

        if (mode == LOGARITHM && num <= 0) {
            error_message = "Cannot perform ";
            error_message += func;
            error_message += " on non-positive value ";
            error_message += value_to_string(value);
            return false;
        }

        return true;
    }

    error_message = "Cannot perform ";
    error_message += func;
    error_message += " on non-numerical value ";
    error_message += value_to_string(value);

    return false;
}

static bool cos NATIVE_FUNCTION_HEADERS() {
    Value arg = stack[0];
    Values::number_t num;

    if (!check_number("cos", error_message, arg, num, REAL)) return false;

    result = Value(ValueType::NUMBER, cos(num));
    return true;
}
static bool sin NATIVE_FUNCTION_HEADERS() {
    Value arg = stack[0];
    Values::number_t num;

    if (!check_number("sin", error_message, arg, num, REAL)) return false;

    result = Value(ValueType::NUMBER, sin(num));
    return true;
}
static bool tan NATIVE_FUNCTION_HEADERS() {
    Value arg = stack[0];
    Values::number_t num;

    if (!check_number("tan", error_message, arg, num, REAL)) return false;

    result = Value(ValueType::NUMBER, tan(num));
    return true;
}

#include <stdio.h>
#define LOG2FF(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

// Fast floored log2 (log2ff). E.g., log2ff(33) is 5
// Errors on negative or 
static bool log2ff NATIVE_FUNCTION_HEADERS() {
    Value arg = stack[0];
    Values::number_t num;

    if (!check_number("log2ff", error_message, arg, num, LOGARITHM)) return false;

    double floored = floor(num);
    if (num != floored) {
        error_message = "Cannot take log2ff of non-integer number ";
        error_message += std::to_string(num);
        return false;
    }
    uint64_t converted = static_cast<uint64_t>(floored);

    result = Value(ValueType::NUMBER, LOG2FF(converted));
    return true;
}

static bool floor NATIVE_FUNCTION_HEADERS() {
    Value arg = stack[0];
    Values::number_t num;

    if (!check_number("floor", error_message, arg, num, REAL)) return false;

    result = Value(ValueType::NUMBER, floor(num));
    return true;
}

Value Natives::create_math_namespace() {
    std::unordered_map<std::string, Value> *Math = new std::unordered_map<std::string, Value>({
        { "cos", Values::Value(
            Values::native_method_t{ .func = cos, .number_arguments = 1 }
        ) },
        { "sin", Values::Value(
            Values::native_method_t{ .func = sin, .number_arguments = 1 }
        ) },
        { "tan", Values::Value(
            Values::native_method_t{ .func = tan, .number_arguments = 1 }
        ) },

        { "log2ff", Values::Value(
            Values::native_method_t{ .func = log2ff, .number_arguments = 1 }
        ) },

        { "floor", Values::Value(
            Values::native_method_t{ .func = floor, .number_arguments = 1 }
        ) },

        { "e", Values::Value(ValueType::NUMBER, 2.7182818284590452353602874713527) },
        { "PI", Values::Value(ValueType::NUMBER, 3.141592653589793238462643383279 ) }
    });
    return Value(new Object(Math));
};
