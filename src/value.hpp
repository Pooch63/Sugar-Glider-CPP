#ifndef _SGCPP_VALUE_HPP
#define _SGCPP_VALUE_HPP

#include "globals.hpp"
#include "operations.hpp"

#ifdef DEBUG
// For inline value functions
#include <cassert>
#endif

namespace Values {
    class Value;

    enum ValueType {
        STRING,
        NUMBER,
        TRUE,
        FALSE,

        NATIVE_FUNCTION
    };
    typedef double number_t;
    typedef void (*native_function_t)(Value* result);

    union value_mem_t {
        number_t number;
        std::string* str;
        native_function_t native;
    };

    class Value {
        private:
            ValueType type;
            value_mem_t value;

            // Whether or not the value class is responsible for freeing the payload
            bool free_payload = true;

        public:
            /* For strings */
            Value(ValueType type, std::string* str);
            /* For numbers */
            Value(ValueType type, number_t number);
            /* For native functions */
            Value(ValueType type, native_function_t native);

            /* For literals: true, false, null */
            Value(ValueType type);

            std::string to_debug_string() const;
            
            inline ValueType get_type() const { return this->type; };
            inline number_t get_number() const {
                #ifdef DEBUG
                assert(this->type == ValueType::NUMBER);
                #endif
                return this->value.number;
            }
            inline std::string* get_string() const {
                #ifdef DEBUG
                assert(this->type == ValueType::STRING);
                #endif
                return this->value.str;
            }

            inline void mark_payload() { this->free_payload = true; }

            bool is_numerical() const;
            Values::number_t to_number() const;
    };

    /* Calculation functions. Return nullptr if there was an error, otherwise a pointer to a heap-allocated value if the operation was successful.
        Meant for optimizations, since these functions doesn't create an error if there is one. */
    Value* bin_op(Operations::BinOpType type, Value a, Value b);
    Value* unary_op(Operations::UnaryOpType type, Value arg);
}

#endif