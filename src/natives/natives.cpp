#include "../runtime/runtime.hpp"
#include "../time-utils.hpp"

using namespace Natives;
using namespace Values;

using Values::Value;

bool println NATIVE_FUNCTION_HEADERS() {
    std::cout << Values::value_to_string(stack[0]) << std::endl;

    return true;
}

bool clock NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(Values::NUMBER, time_in_nanoseconds());

    return true;
}

#include "array.hpp"
#include "date.hpp"
#include "math.hpp"

Native::Native(const char *name, Values::Value value) :
    native_name(name), value(value) {};

uint Natives::get_native_index(std::string native_name) {
    return name_to_native_index[native_name];
};
std::unordered_map<std::string, uint> Natives::name_to_native_index = {
    { "println", 0 },
    { "clock", 1 },
    { "Array", 2 },
    { "Date", 3 },
    { "Math", 4 }
};

void Natives::create_natives(std::array<Value, native_count> &natives) {
    natives[0] = Values::Value(
        Values::native_method_t{ .func = println, .number_arguments = 1 }
    );
    natives[1] = Values::Value(
        Values::native_method_t{ .func = clock, .number_arguments = 0 }
    );

    natives[2] = Natives::create_array_namespace();
    natives[3] = Natives::create_date_namespace();
    natives[4] = Natives::create_math_namespace();
};
