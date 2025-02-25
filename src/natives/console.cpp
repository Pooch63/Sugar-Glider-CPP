#include "console.hpp"
#include "natives.hpp"
#include "../memory.hpp"

using namespace Values;

static bool print NATIVE_FUNCTION_HEADERS() {
    std::cout << Values::value_to_string(stack[0]);

    return true;
}
static bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << Values::value_to_string(stack[0]) << std::endl;

    return true;
}

Values::Value create_string_value(const char *str) {
    std::string *str_val = Allocate<std::string>::create(str);
    Object *obj = Allocate<Object>::create(str_val);
    return value_from_object(obj);
}

Value Natives::create_console_namespace() {
    std::unordered_map<std::string, Value> *FG = new std::unordered_map<std::string, Value>({
        { "black", create_string_value("\x1b[0;30m") },
        { "red", create_string_value("\x1b[0;31m") },
        { "green", create_string_value("\x1b[0;32m") },
        { "yellow", create_string_value("\x1b[0;33m") },
        { "blue", create_string_value("\x1b[0;34m") },
        { "purple", create_string_value("\x1b[0;35m") },
        { "cyan", create_string_value("\x1b[0;36m") },
        { "white", create_string_value("\x1b[0;37m") }
    });
    Object *fg_obj = Allocate<Object>::create(FG);

    std::unordered_map<std::string, Value> *BG = new std::unordered_map<std::string, Value>({
        { "black", create_string_value("\x1b[40m") },
        { "red", create_string_value("\x1b[41m") },
        { "green", create_string_value("\x1b[42m") },
        { "yellow", create_string_value("\x1b[43m") },
        { "blue", create_string_value("\x1b[44m") },
        { "purple", create_string_value("\x1b[45m") },
        { "cyan", create_string_value("\x1b[46m") },
        { "white", create_string_value("\x1b[47m") }
    });
    Object *bg_obj = Allocate<Object>::create(BG);

    std::unordered_map<std::string, Value> *Console = new std::unordered_map<std::string, Value>({
        { "print", Values::Value(
            Values::native_method_t{ .func = print, .number_arguments = 1 }
        ) },
        { "println", Values::Value(
            Values::native_method_t{ .func = println, .number_arguments = 1 }
        ) },

        { "fg", value_from_object(fg_obj) },
        { "bg", value_from_object(bg_obj) },
        { "bold", create_string_value("\x1b[1m") },
        { "underline", create_string_value("\x1b[4m") },
        { "reset", create_string_value("\x1b[0m") }
    });
    Object *array_obj = Allocate<Object>::create(Console);
    return Value(array_obj);
};
