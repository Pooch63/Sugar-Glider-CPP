#include "runtime/runtime.hpp"
#include "utils.hpp"
#include "value.hpp"

#include <bits/stdc++.h>
#include <limits>

namespace Math {
    #include <math.h>
}

#ifdef DEBUG
#include <cassert>
#endif

using namespace Values;

Object::Object(std::string *str) :
    type(ObjectType::STRING), memory(obj_mem_t{ .str = str }) {};
Object::Object(std::vector<Value> *array) :
    type(ObjectType::ARRAY), memory(obj_mem_t{ .array = array }) {};
Object::Object(namespace_t *namespace_) :
    type(ObjectType::NAMESPACE_CONSTANT), memory(obj_mem_t{ .namespace_ = namespace_ }) {}

Object::~Object() {
    switch (this->type) {
        case ObjectType::STRING: delete this->memory.str; break;
        case ObjectType::ARRAY: delete this->memory.array; break;
        case ObjectType::NAMESPACE_CONSTANT: delete this->memory.namespace_;
    }
}

Value::Value(Object *obj) : type(ValueType::OBJ), value(value_mem_t{ .obj = obj }) {}
Value::Value(ValueType type, number_t number) : type(type), value(value_mem_t{ .number = number }) {
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::NUMBER);
    #endif
};
Value::Value(Bytecode::constant_index_t prog_func_index, ValueType type) :
    type(type), value(value_mem_t{ .prog_func_index = prog_func_index })
{
    #ifdef DEBUG_ASSERT
    assert(type == ValueType::PROGRAM_FUNCTION);
    #endif
};
Value::Value(native_method_t native) : type(ValueType::NATIVE_FUNCTION), value(value_mem_t{ .native = native }) {};
Value::Value(ValueType type) : type(type) {};
Value::Value() : type(ValueType::NULL_VALUE) {};

std::string Values::value_to_string(const Value &value) {
    switch (get_value_type(value)) {
        case ValueType::NUMBER: return std::to_string(get_value_number(value));
        case ValueType::TRUE:   return "true";
        case ValueType::FALSE:  return "false";
        case ValueType::NULL_VALUE: return "null";
        case ValueType::PROGRAM_FUNCTION: return "function";
        case ValueType::NATIVE_FUNCTION: return "[native function]";
        case ValueType::OBJ: {
            Object *obj = get_value_object(value);
            switch (obj->type) {
                case ObjectType::STRING: return '"' + *get_value_string(value) + '"';
                case ObjectType::ARRAY: {
                    std::string str = "[ ";
                    bool found_value = false;
                    for (Value value : *get_value_array(value)) {
                        if (found_value) str += ", ";
                        str += value_to_string(value);
                        found_value = true;
                    }
                    str += " ]";
                    return str;
                }
                default: throw sg_assert_error("Unknown object type when making string");
            }
        }
        default:
            throw sg_assert_error("Unknown value to log as string");
    }
};

std::string Values::object_to_debug_string(Object *obj) {
    switch (obj->type) {
        case ObjectType::STRING: {
            std::string trimmed;
            truncate_string(trimmed, 36, *obj->memory.str);
            return '"' + trimmed + '"';
        }
        case ObjectType::ARRAY: {
            std::string str = "[ ";
            bool found_value = false;
            for (Value value : *obj->memory.array) {
                if (found_value) str += ", ";
                str += value_to_debug_string(value);
                found_value = true;
            }
            str += " ]";
            return str;
        }
        default: throw sg_assert_error("Unknown object type when making debug string");
    }
};
std::string Values::value_to_debug_string(const Value &value) {
    if (get_value_type(value) == ValueType::OBJ) return object_to_debug_string(get_value_object(value));
    return value_to_string(value);
};

void Values::free_value_if_object(Value &value) {
    if (get_value_type(value) != ValueType::OBJ) return;
    delete get_value_object(value);
};
Object *Values::safe_get_value_object(const Value &value) {
    if (get_value_type(value) != ValueType::OBJ) return nullptr;
    return get_value_object(value);
}

bool Values::value_is_truthy(const Value &value) {
    switch (get_value_type(value)) {
        case ValueType::NULL_VALUE: return false;
        case ValueType::TRUE: return true;
        case ValueType::FALSE: return false;
        case ValueType::NUMBER: return get_value_type(value) != 0;
        case ValueType::OBJ: {
            Object *obj = get_value_object(value);
            switch (obj->type) {
                case ObjectType::STRING: return get_value_string(value)->size() > 0;
                case ObjectType::ARRAY: return get_value_array(value)->size() > 0;
                default: throw sg_assert_error("Unknown object type when determining value truth");
            }
        }
        case ValueType::NATIVE_FUNCTION: return true;
        default:
            throw sg_assert_error("Unknown value to get truthy value from");
    }
};
bool Values::value_is_numerical(const Value &value) {
    return get_value_type(value) == ValueType::NUMBER ||
        get_value_type(value) == ValueType::TRUE ||
        get_value_type(value) == ValueType::FALSE;
};
Values::number_t Value::to_number() const {
    switch (this->type) {
        case ValueType::NUMBER: return this->value.number;
        case ValueType::TRUE: return static_cast<Values::number_t>(1);
        case ValueType::FALSE: return static_cast<Values::number_t>(0);
        default:
            throw sg_assert_error("Tried to convert non-numeric value type to number");
    }
}

bool Values::values_are_equal(const Value &a, const Value &b) {
    if (get_value_type(a) != get_value_type(b)) return false;
    switch (get_value_type(a)) {
        case ValueType::OBJ: {
            Object *obj_a = get_value_object(a);
            Object *obj_b = get_value_object(b);
            if (obj_a->type != obj_b->type) return false;

            switch (obj_a->type) {
                case ObjectType::STRING: return *get_value_string(a) == *get_value_string(b);
                case ObjectType::ARRAY: return get_value_array(a) == get_value_array(b);
                default: throw sg_assert_error("Unknown object type when determining object equality");
            }
        }
        case ValueType::NATIVE_FUNCTION: return get_value_native_function(a).func == get_value_native_function(b).func;
        default: return true;
    }
};

// Calculate a % b. Get the number you must subtract from "a" in order to make
// "a" a multiple of b.
static Values::number_t mod(Values::number_t a, Values::number_t b) {
    if (b == 0) {
        if ((a > 0) ^ (b > 0)) return -std::numeric_limits<double>::infinity();
        return std::numeric_limits<double>::infinity();
    }
    if (a < 0) return b - fmod(Math::fabs(a), b);

    return fmod(a, b);
}

bool Values::bin_op(
    Operations::BinOpType type,
    Value a,
    Value b,
    Value *result,
    std::string *error
) {
    using Operations::BinOpType;

    if (
        type == BinOpType::BINOP_ADD &&
        get_value_type(a) == ValueType::OBJ &&
        get_value_type(b) == ValueType::OBJ
    ) {
        Object *obj_a = get_value_object(a);
        Object *obj_b = get_value_object(b);

        if (obj_a->type == ObjectType::STRING && obj_b->type == ObjectType::STRING) {
            std::string *concat = new std::string(*get_value_string(a) + *get_value_string(b));
            Object *obj = new Object(concat);
            *result = Value(obj);
            return true;
        }
    }
    if (type == BinOpType::BINOP_NOT_EQUAL_TO) {
        *result = Value(values_are_equal(a, b) ? ValueType::FALSE : ValueType::TRUE);
        return true;
    }
    if (type == BinOpType::BINOP_EQUAL_TO) {
        *result = Value(values_are_equal(a, b) ? ValueType::TRUE : ValueType::FALSE);
        return true;
    }

    if (!value_is_numerical(a) || !value_is_numerical(b)) {
        if (error != nullptr) {
            *error = "Cannot perform binary operation ";
            *error += Operations::bin_op_to_string(type);
            *error += " on values ";
            *error += value_to_string(a);
            *error += " and ";
            *error += value_to_string(b);
        }
        return false;
    }

    Values::number_t first = a.to_number(),
        second = b.to_number();

    int firstI, secondI;

    if (Operations::bin_op_is_bitwise_operator(type)) {
        /* Make sure the numbers are both integers. */
        firstI = floor(first);
        secondI = floor(second);

        /* Bitwise operation error on floats */
        if (first != firstI && second != secondI) {
            if (error != nullptr) {
                *error = "Cannot perform bitwise operation ";
                *error += Operations::bin_op_to_string(type);
                *error += " on non-integer values ";
                *error += value_to_string(a);
                *error += " and ";
                *error += value_to_string(b);
            }
            return false;
        }
    }

    switch (type) {
        case BinOpType::BINOP_ADD:
            *result = Value(ValueType::NUMBER, first + second); break;
        case BinOpType::BINOP_SUB:
            *result = Value(ValueType::NUMBER, first - second); break;
        case BinOpType::BINOP_MUL:
            *result = Value(ValueType::NUMBER, first * second); break;
        case BinOpType::BINOP_DIV:
            *result = Value(ValueType::NUMBER, first / second); break;
        case BinOpType::BINOP_MOD:
            *result = Value(ValueType::NUMBER, mod(first, second)); break;
        case BinOpType::BINOP_LESS_THAN:
            *result = Value(first < second ? ValueType::TRUE : ValueType::FALSE); break;
        case BinOpType::BINOP_GREATER_THAN:
            *result = Value(first > second ? ValueType::TRUE : ValueType::FALSE); break;
        default:
            throw sg_assert_error("Tried to compute unknown binary operation on two values");
    }

    return true;
};
bool Values::unary_op(Operations::UnaryOpType type, Value arg, Value *result, std::string *error) {
    using Operations::UnaryOpType;

    if (!value_is_numerical(arg)) {
        if (error != nullptr) {
            *error = "Cannot perform unary operation ";
            *error += Operations::unary_op_to_string(type);
            *error += " on value ";
            *error += value_to_string(arg);
        }
        return false;
    }

    Values::number_t num = arg.to_number();

    int argI;

    if (Operations::unary_is_bitwise_operator(type)) {
        /* Make sure the numbers is an integer. */
        argI = floor(num);

        /* Bitwise operation error on floats */
        if (num != argI) {
            if (error != nullptr) {
                *error = "Cannot perform bitwise operation ";
                *error += Operations::unary_op_to_string(type);
                *error += " on non-integer value ";
                *error += value_to_string(arg);
            }
            return false;
        }
    }

    switch (type) {
        case UnaryOpType::UNARY_NEGATE:
            *result = Value(ValueType::NUMBER, -num); break;
        default:
            throw sg_assert_error("Tried to compute unknown unary operation on value");
    }

    return true;
};
