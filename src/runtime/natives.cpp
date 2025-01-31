#include "natives.hpp"
#include "../utils.hpp"

using namespace Natives;

#define NATIVE_FUNCTION_HEADERS() ( \
    [[maybe_unused]] const Value * const start, \
    [[maybe_unused]] uint stack_size, \
    [[maybe_unused]] Value &result, \
    [[maybe_unused]] std::string &error_message)

using Values::Value;

bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << start[0].to_string() << std::endl;

    return true;
}

bool clock NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(Values::NUMBER, time_in_nanoseconds());

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
    { "clock", 2 }
};
void Natives::create_natives(std::array<Value, native_count> &natives) {
    natives[0] = Values::Value(Values::NUMBER, 3.14159265358979323);

    natives[1] = Values::Value(
        Values::native_method_t{ .func = println, .number_arguments = 1 }
    );
    natives[2] = Values::Value(
        Values::native_method_t{ .func = clock, .number_arguments = 0 }
    );
};
