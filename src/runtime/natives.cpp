#include "natives.hpp"
#include "../utils.hpp"

using namespace Natives;
using namespace Values;

#define NATIVE_FUNCTION_HEADERS() ( \
    [[maybe_unused]] const Value * const start, \
    [[maybe_unused]] uint stack_size, \
    [[maybe_unused]] Value &result, \
    [[maybe_unused]] std::string &error_message)

using Values::Value;

bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << Values::value_to_string(start[0]) << std::endl;

    return true;
}

bool clock NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(Values::NUMBER, time_in_nanoseconds());

    return true;
}

bool length NATIVE_FUNCTION_HEADERS() {
    ValueType type = get_value_type(start[0]);
    if (type == ValueType::STRING) {
        result = Values::Value(Values::NUMBER, get_value_string(start[0])->size());
    }
    else if (type == ValueType::ARRAY) {
        result = Values::Value(Values::NUMBER, get_value_array(start[0])->size());
    }
    else {
        error_message = "Cannot get length of value ";
        error_message += value_to_string(start[0]);
        error_message += " - it is not an array or string";
        return false;
    }

    return true;
}
bool append NATIVE_FUNCTION_HEADERS() {
    ValueType appendee_type = get_value_type(start[0]);
    Value added_type = start[1];

    if (appendee_type == ValueType::STRING) {
        if (get_value_type(added_type) != ValueType::STRING) {
            error_message = "Cannot add non-string value ";
            error_message += value_to_string(added_type);
            error_message += " to string ";
            error_message += value_to_string(start[0]);
            return false;
        }
        result = Value(ValueType::STRING, new std::string(*get_value_string(start[0]) + *get_value_string(start[1])));
        return true;
    }
    else if (appendee_type == ValueType::ARRAY) {
       get_value_array(start[0])->push_back(added_type);
       result = start[0];
       return true;
    }
    else {
        error_message = "Cannot get append to value ";
        error_message += value_to_string(start[0]);
        error_message += " - it is not an array or string";
        return false;
    }

    return true;
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
