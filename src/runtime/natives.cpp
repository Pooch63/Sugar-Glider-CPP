#include "natives.hpp"

using namespace Natives;

const char *Natives::native_names[native_count] = {
    "PI",
    "println"
};

#define NATIVE_FUNCTION_HEADERS() ( \
    [[maybe_unused]] Value *start, \
    [[maybe_unused]] uint stack_size, \
    [[maybe_unused]] Value &result, \
    [[maybe_unused]] std::string &error_message)

using Values::Value;
bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << start[0].to_debug_string() << std::endl;

    return true;
}

Native::Native(const char *name, Values::Value value) :
    native_name(name), value(value) {};

uint Natives::get_native_index(std::string native_name) {
    return name_to_native_index[native_name];
};
std::unordered_map<std::string, uint> Natives::name_to_native_index = {
    { "PI", 0 },
    { "println", 1 }
};
void Natives::create_natives(std::array<Native, native_count> &natives) {
    natives[0] = Native("PI", Values::Value(Values::NUMBER, 3.14159265358979323));

    natives[1] = Native("println", Values::Value(
        Values::native_method_t{ .func = println, .number_arguments = 1 }
    ));
};
