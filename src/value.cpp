#include "value.hpp"

using namespace Values;

Value::Value(number_t number) : type(ValueType::NUMBER), value(value_mem_t{ .number = number }) {};