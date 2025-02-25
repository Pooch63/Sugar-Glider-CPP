#include "date.hpp"
#include "natives.hpp"
#include "../memory.hpp"

#include "../runtime/runtime.hpp"
#include "../time-utils.hpp"

using namespace Values;

bool timezoneName NATIVE_FUNCTION_HEADERS() {
    std::string *name_container = runtime.create<std::string>(get_timezone_name());
    Object *obj = runtime.create<Object>(name_container);
    runtime.add_object(obj);
    result = value_from_object(obj);
    return true;
}

Value Natives::create_date_namespace() {
    std::unordered_map<std::string, Value> *Date = new std::unordered_map<std::string, Value>({
        { "timezoneName", Values::Value(
            Values::native_method_t{ .func = timezoneName, .number_arguments = 0 }
        ) }
    });
    Object *array_obj = Allocate<Object>::create(Date);
    return Value(array_obj);
};
