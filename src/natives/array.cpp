#include "array.hpp"
#include "natives.hpp"

using namespace Values;

static bool append NATIVE_FUNCTION_HEADERS() {
    Value appendee = stack[0];
    Value added_type = stack[1];

    Object *obj = safe_get_value_object(appendee);

    if (obj->type == ObjectType::ARRAY) {
       get_value_array(appendee)->push_back(added_type);
       return true;
    }
    else {
        error_message = "Cannot append to value ";
        error_message += value_to_string(appendee);
        error_message += " - it is not an array";
        return false;
    }
}
static bool length NATIVE_FUNCTION_HEADERS() {
    Value top = stack[0];
    Object *obj = safe_get_value_object(top);

    if (obj == nullptr || obj->type != ObjectType::ARRAY) {
        error_message = "Cannot get length of value ";
        error_message += value_to_string(top);
        error_message += " - it is not an array or string";
        return false;
    }

    if (obj->type == ObjectType::ARRAY) {
        result = Values::Value(Values::NUMBER, get_value_array(top)->size());
    }
    return true;
}

Value Natives::create_array_namespace() {
    std::unordered_map<std::string, Value> *Array = new std::unordered_map<std::string, Value>({
        { "append", Values::Value(
            Values::native_method_t{ .func = append, .number_arguments = 2 }
        ) },
        { "length", Values::Value(
            Values::native_method_t{ .func = length, .number_arguments = 1 }
        ) }
    });
    return Value(new Object(Array));
};
