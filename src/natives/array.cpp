#include "array.hpp"
#include "natives.hpp"

using namespace Values;

/**
 * @param {const char*} process - Process to describe in the error message
 * @param {std::string&} error_message - Error to update
 * @param {Values::Value&} value - Value to check for being an array
 * @return {Object*} - Pointer if success, nullptr if error message was shown 
 */
static Object *check_array(const char *process, std::string &error_message, Values::Value &value) {
    Object *obj = get_value_object(value);
    if (obj == nullptr || obj->type != ObjectType::ARRAY) {
        error_message = "Cannot ";
        error_message += process;
        error_message += " value ";
        error_message += object_to_string(obj);
        error_message += " -- it is not an array";
        return nullptr;
    }
    return obj;
}

static bool append NATIVE_FUNCTION_HEADERS() {
    Value appendee = stack[0];
    Value added_type = stack[1];

    Object *obj = check_array("append to", error_message, appendee);
    if (!obj) return false;

    get_value_array(appendee)->push_back(added_type);
    return true;
}
static bool includes NATIVE_FUNCTION_HEADERS() {
    Value array_value = stack[0];
    Value value = stack[1];

    Object *obj = check_array("check include in", error_message, array_value);
    if (!obj) return false;

    std::vector<Value> *array = obj->memory.array;
    for (Value element : *array) {
        if (values_are_equal(value, element)) {
            result = Value(ValueType::TRUE);
            return true;
        }
    }
    result = Value(ValueType::FALSE);
    return false;
}
static bool length NATIVE_FUNCTION_HEADERS() {
    Value array = stack[0];
    Object *obj = check_array("get length of", error_message, array);
    if (!obj) return false;

    result = Values::Value(Values::NUMBER, get_value_array(array)->size());
    return true;
}

Value Natives::create_array_namespace() {
    std::unordered_map<std::string, Value> *Array = new std::unordered_map<std::string, Value>({
        { "append", Values::Value(
            Values::native_method_t{ .func = append, .number_arguments = 2 }
        ) },
        { "includes", Values::Value(
            Values::native_method_t{ .func = includes, .number_arguments = 2 }
        ) },
        { "length", Values::Value(
            Values::native_method_t{ .func = length, .number_arguments = 1 }
        ) }
    });
    return Value(new Object(Array));
};
