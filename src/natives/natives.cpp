#include "../runtime/runtime.hpp"
#include "../time-utils.hpp"

using namespace Natives;
using namespace Values;

using Values::Value;

bool clock NATIVE_FUNCTION_HEADERS() {
    result = Values::Value(Values::NUMBER, time_in_nanoseconds());

    return true;
}

#include "array.hpp"
#include "console.hpp"
#include "date.hpp"
#include "math.hpp"

Native::Native(const char *name, Values::Value value) :
    native_name(name), value(value) {};

uint Natives::get_native_index(std::string native_name) {
    return name_to_native_index[native_name];
};
std::unordered_map<std::string, uint> Natives::name_to_native_index = {
    { "Console", 0 },
    { "clock", 1 },
    { "Array", 2 },
    { "Date", 3 },
    { "Math", 4 }
};

void Natives::create_natives(std::array<Value, native_count> &natives) {
    natives[0] = Natives::create_console_namespace();
    natives[1] = Values::Value(
        Values::native_method_t{ .func = clock, .number_arguments = 0 }
    );

    natives[2] = Natives::create_array_namespace();
    natives[3] = Natives::create_date_namespace();
    natives[4] = Natives::create_math_namespace();
};
