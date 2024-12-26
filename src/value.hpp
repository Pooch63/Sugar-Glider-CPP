#ifndef _SGCPP_VALUE_HPP
#define _SGCPP_VALUE_HPP

#include "globals.hpp"
#include "operations.hpp"

#ifdef DEBUG
// For inline value functions
#include <cassert>
#endif

namespace Values {
    enum ValueType {
        NUMBER,
        TRUE,
        FALSE
    };
    typedef double number_t;

    union value_mem_t {
        number_t number;
    };

    class Value {
        private:
            ValueType type;
            value_mem_t value;

        public:
            /* For numbers */
            Value(ValueType type, number_t number);

            /* For literals: true, false, null */
            Value(ValueType type);
            
            inline ValueType get_type() const { return this->type; };
            inline number_t get_number() const {
                #ifdef DEBUG
                assert(this->type == ValueType::NUMBER);
                #endif
                return this->value.number;
            }

            bool is_numerical() const;
            Values::number_t to_number() const;
    };

    /* Calculation functions. Return nullptr if there was an error, otherwise a pointer to a heap-allocated value if the operation was successful.
        Meant for optimizations, since these functions doesn't create an error if there is one. */
    Value* bin_op(Operations::BinOpType type, Value a, Value b);
    Value* unary_op(Operations::UnaryOpType type, Value arg);
}

#endif