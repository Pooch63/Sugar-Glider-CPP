#include "runtime.hpp"
#include "../utils.hpp"

using namespace Natives;
using namespace Values;

#define NATIVE_FUNCTION_HEADERS() ( \
    [[maybe_unused]] const Value * const stack, \
    [[maybe_unused]] uint stack_size, \
    [[maybe_unused]] Value &result, \
    [[maybe_unused]] Runtime &runtime, \
    [[maybe_unused]] std::string &error_message)

using Values::Value;

bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << Values::value_to_string(stack[0]) << std::endl;

    return true;
}

bool clock NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(Values::NUMBER, time_in_nanoseconds());

    return true;
}

bool length NATIVE_FUNCTION_HEADERS() {
    Value top = stack[0];
    ValueType type = get_value_type(top);
    if (type == ValueType::STRING) {
        result = Values::Value(Values::NUMBER, get_value_string(top)->size());
    }
    else if (type == ValueType::ARRAY) {
        result = Values::Value(Values::NUMBER, get_value_array(top)->size());
    }
    else {
        error_message = "Cannot get length of value ";
        error_message += value_to_string(top);
        error_message += " - it is not an array or string";
        return false;
    }

    return true;
}
bool append NATIVE_FUNCTION_HEADERS() {
    Value appendee = stack[0];
    ValueType appendee_type = get_value_type(appendee);
    Value added_type = stack[1];

    if (appendee_type == ValueType::ARRAY) {
       get_value_array(appendee)->push_back(runtime.get_runtime_value(added_type));
       return true;
    }
    else {
        error_message = "Cannot append to value ";
        error_message += value_to_string(appendee);
        error_message += " - it is not an array";
        return false;
    }
}

Native::Native(const char *name, Values::Value value) :
    native_name(name), value(value) {};

uint Natives::get_native_index(std::string native_name) {
    return name_to_native_index[native_name];
};
std::unordered_map<std::string, uint> Natives::name_to_native_index = {
    { "PI", 0 },
    { "println", 1 },
    { "clock", 2 },
    { "length", 3 },
    { "append", 4 }
};
void Natives::create_natives(std::array<Value, native_count> &natives) {
    natives[0] = Values::Value(Values::NUMBER, 3.14159265358979323);

    natives[1] = Values::Value(
        Values::native_method_t{ .func = println, .number_arguments = 1 }
    );
    natives[2] = Values::Value(
        Values::native_method_t{ .func = clock, .number_arguments = 0 }
    );
    natives[3] = Values::Value(
        Values::native_method_t{ .func = length, .number_arguments = 1 }
    );
    natives[4] = Values::Value(
        Values::native_method_t{ .func = append, .number_arguments = 2 }
    );
};
