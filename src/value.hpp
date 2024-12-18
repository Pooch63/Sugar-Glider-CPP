#ifndef _SGCPP_VALUE_HPP
#define _SGCPP_VALUE_HPP

#ifdef DEBUG
// For inline value functions
#include <cassert>
#endif

namespace Values {
    enum ValueType {
        NUMBER
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
            /* Constructors automatically intiailize the type */
            Value(number_t number);

            
            inline number_t get_number() const {
                #ifdef DEBUG
                assert(this->type == ValueType::NUMBER);
                #endif
                return this->value.number;
            }
    };
}

#endif