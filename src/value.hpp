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
        NULL_VALUE,

        NATIVE_FUNCTION
    };
    typedef double number_t;
    /**
     * @param {Value*} - start of stack
     * @param {uint} - number of arguments passed
     * @param {Value&} - return value. MUST be updated
     * @param {std::string&} - reference to error message that may be updated
     * @return {bool} - True if okay, false if error
    */
    typedef bool (*native_function_t)(const Value * const start, uint arg_count, Value &result, std::string &error_message);

    struct native_method_t {
        native_function_t func;
        int number_arguments;
    };

    union value_mem_t {
        number_t number;
        std::string* str;
        native_method_t native;
    };

    class Value {
        private:
            ValueType type;
            value_mem_t value;

            // Whether or not the value class is responsible for freeing the payload
            bool should_free_payload = true;

        public:
            /* For strings */
            Value(ValueType type, std::string* str);
            /* For numbers */
            Value(ValueType type, number_t number);
            /* For native functions */
            Value(native_method_t native);

            /* For literals: true, false, null */
            Value(ValueType type);

            /* Default constructor so that Value can be part of arrays, hashmap, etc. */
            Value();

            std::string to_string() const;
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
            inline native_method_t get_native_function() const {
                #ifdef DEBUG
                assert(this->type == ValueType::NATIVE_FUNCTION);
                #endif
                return this->value.native;
            }

            inline void mark_payload() { this->should_free_payload = true; }
            void free_payload();

            bool is_truthy() const;
            bool is_numerical() const;
            Values::number_t to_number() const;
    };

    /**
     * @param {BinOpType} type
     * @param {Value} a
     * @param {Value} b
     * @param {Value*} result - The place to add the result
     * @param {std::string*} place to write error message. If you don't want an error, pass nullptr
     * @return {bool} - True if ok, false if error
     */
    bool bin_op(Operations::BinOpType type, Value a, Value b, Value *result, std::string *error);
    /**
     * @param {BinOpType} type
     * @param {Value} argument
     * @param {Value*} result - The place to add the result
     * @param {std::string*} place to write error message. If you don't want an error, pass nullptr
     * @return {bool} - True if ok, false if error
     */
    bool unary_op(Operations::UnaryOpType type, Value a, Value *result, std::string *error);
}

#endif