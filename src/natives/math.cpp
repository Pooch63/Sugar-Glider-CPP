#include "math.hpp"
#include "natives.hpp"
#include "../memory.hpp"
#include "../utils.hpp"

#include <math.h>
#include <stdio.h>

#include "array.hpp"
#include "natives.hpp"

using namespace Values;

// Possible check ranges
enum CheckMode {
    // All numbers
    REAL,
    // Input must be greater than 0
    LOGARITHM,
    // Input must be greater than or equal to 0
    NON_NEGATIVE
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

using one_param_t = double(*)(double) noexcept;
using two_param_t = double(*)(double, double) noexcept;

static inline bool math_function_one_param(
    const char *process,
    const Value * const stack,
    Value &result,
    std::string &error_message,
    CheckMode mode,
    one_param_t func
) {
    Values::number_t num;
    if (!check_number(process, error_message, stack[0], num, mode)) return false;

    result = Value(ValueType::NUMBER, func(num));
    return true;
}
template<typename func_t = two_param_t>
static inline bool math_function_two_param(
    const char *process,
    const Value * const stack,
    Value &result,
    std::string &error_message,
    CheckMode mode_a,
    CheckMode mode_b,
    func_t func
) {
    Values::number_t num_a;
    Values::number_t num_b;

    if (!check_number(process, error_message, stack[0], num_a, mode_a)) return false;
    if (!check_number(process, error_message, stack[1], num_b, mode_b)) return false;

    result = Value(ValueType::NUMBER, func(num_a, num_b));
    return true;
}

static bool sg_abs NATIVE_FUNCTION_HEADERS() {
    Values::number_t num;
    if (!check_number("abs", error_message, stack[0], num, REAL)) return false;

    result = Value(ValueType::NUMBER, num >= 0 ? num : -num);
    return true;
}
static bool sg_floor NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "floor",
        stack,
        result,
        error_message,
        REAL,
        floor
    );
}
static bool sg_ceil NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "ceil",
        stack,
        result,
        error_message,
        REAL,
        ceil
    );
}

static bool sg_max NATIVE_FUNCTION_HEADERS() {
    return math_function_two_param<const double &(*)(const double&, const double&)>(
        "max",
        stack,
        result,
        error_message,
        REAL,
        REAL,
        std::max<double>
    );
}
static bool sg_min NATIVE_FUNCTION_HEADERS() {
    return math_function_two_param<const double &(*)(const double&, const double&)>(
        "min",
        stack,
        result,
        error_message,
        REAL,
        REAL,
        std::min<double>
    );
}

// Define the distribution range
std::uniform_real_distribution<> decimal_dis(0.0, 1.0);
static bool sg_random NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(ValueType::NUMBER, decimal_dis(Random::rng));
    return true;
}

static bool sg_acos NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "acos",
        stack,
        result,
        error_message,
        REAL,
        acos
    );
}
static bool sg_asin NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "asin",
        stack,
        result,
        error_message,
        REAL,
        asin
    );
}
static bool sg_atan NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "atan",
        stack,
        result,
        error_message,
        REAL,
        atan
    );
}

static bool sg_cos NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "cos",
        stack,
        result,
        error_message,
        REAL,
        cos
    );
}
static bool sg_sin NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "sin",
        stack,
        result,
        error_message,
        REAL,
        sin
    );
}
static bool sg_tan NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "tan",
        stack,
        result,
        error_message,
        REAL,
        tan
    );
}

// Logarithms error on non-positive numbers
static bool sg_log10 NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "log10",
        stack,
        result,
        error_message,
        LOGARITHM,
        log10
    );
}
static bool sg_logE NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "logE",
        stack,
        result,
        error_message,
        LOGARITHM,
        log
    );
}
static bool sg_log2 NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "log2",
        stack,
        result,
        error_message,
        LOGARITHM,
        log2
    );
}
// Fast floored log2 (log2ff). E.g., log2ff(33) is 5
#define LOG2FF(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))
static bool sg_log2ff NATIVE_FUNCTION_HEADERS() {
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

static bool sg_pow NATIVE_FUNCTION_HEADERS() {
    return math_function_two_param(
        "pow",
        stack,
        result,
        error_message,
        NON_NEGATIVE,
        REAL,
        pow
    );
}
static bool sg_sqrt NATIVE_FUNCTION_HEADERS() {
    return math_function_one_param(
        "sqrt",
        stack,
        result,
        error_message,
        NON_NEGATIVE,
        sqrt
    );
}

Value Natives::create_math_namespace() {
    std::unordered_map<std::string, Value> *Math = new std::unordered_map<std::string, Value>({
        { "abs", Values::Value(
            Values::native_method_t{ .func = sg_abs, .number_arguments = 1 }
        ) },
        { "ceil", Values::Value(
            Values::native_method_t{ .func = sg_ceil, .number_arguments = 1 }
        ) },
        { "floor", Values::Value(
            Values::native_method_t{ .func = sg_floor, .number_arguments = 1 }
        ) },
        { "max", Values::Value(
            Values::native_method_t{ .func = sg_max, .number_arguments = 2 }
        ) },
        { "min", Values::Value(
            Values::native_method_t{ .func = sg_min, .number_arguments = 2 }
        ) },

        { "random", Values::Value(
            Values::native_method_t{ .func = sg_random, .number_arguments = 0 }
        ) },
        
        { "acos", Values::Value(
            Values::native_method_t{ .func = sg_acos, .number_arguments = 1 }
        ) },
        { "asin", Values::Value(
            Values::native_method_t{ .func = sg_asin, .number_arguments = 1 }
        ) },
        { "atan", Values::Value(
            Values::native_method_t{ .func = sg_atan, .number_arguments = 1 }
        ) },

        { "cos", Values::Value(
            Values::native_method_t{ .func = sg_cos, .number_arguments = 1 }
        ) },
        { "sin", Values::Value(
            Values::native_method_t{ .func = sg_sin, .number_arguments = 1 }
        ) },
        { "tan", Values::Value(
            Values::native_method_t{ .func = sg_tan, .number_arguments = 1 }
        ) },

        { "log10", Values::Value(
            Values::native_method_t{ .func = sg_log10, .number_arguments = 1 }
        ) },
        { "logE", Values::Value(
            Values::native_method_t{ .func = sg_logE, .number_arguments = 1 }
        ) },
        { "log2", Values::Value(
            Values::native_method_t{ .func = sg_log2, .number_arguments = 1 }
        ) },
        { "log2ff", Values::Value(
            Values::native_method_t{ .func = sg_log2ff, .number_arguments = 1 }
        ) },

        { "sqrt", Values::Value(
            Values::native_method_t{ .func = sg_sqrt, .number_arguments = 1 }
        ) },
        { "pow", Values::Value(
            Values::native_method_t{ .func = sg_pow, .number_arguments = 2 }
        ) },

        { "E", Values::Value(ValueType::NUMBER, 2.7182818284590452353602874713527) },
        { "PI", Values::Value(ValueType::NUMBER, 3.141592653589793238462643383279 ) }
    });
    Object *math_obj = Allocate<Object>::create(Math);
    return Value(math_obj);
};
