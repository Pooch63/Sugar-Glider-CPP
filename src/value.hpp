#ifndef _SGCPP_VALUE_HPP
#define _SGCPP_VALUE_HPP

#include "globals.hpp"
#include "ir/bytecode.hpp"
#include "operations.hpp"

#ifdef DEBUG
// For inline value functions
#include <cassert>
#endif

struct RuntimeValue;
class Runtime;

namespace Values {
    class Value;

    enum ValueType {
        ARRAY,
        STRING,
        NUMBER,
        TRUE,
        FALSE,
        NULL_VALUE,

        PROGRAM_FUNCTION,
        NATIVE_FUNCTION
    };
    /**
     * @param {Value*} - start of stack
     * @param {uint} - number of arguments passed
     * @param {Value&} - return value. MUST be updated
     * @param {std::string&} - reference to error message that may be updated
     * @return {bool} - True if okay, false if error
    */
    typedef bool (*native_function_t)(const Value * const start, uint arg_count, Value &result, Runtime &runtime, std::string &error_message);

    struct native_method_t {
        native_function_t func;
        int number_arguments;
    };

    class Value;

    /* With arrays, the value class is responsible for the container. It is NOT
        responsible for the elements inside, nor their free. */
    union value_mem_t {
        number_t number;
        std::string *str;
        native_method_t native;
        Bytecode::constant_index_t prog_func_index;
        std::vector<RuntimeValue*> *array;
    };

    class Value {
        private:
            ValueType type;
            value_mem_t value;

            // Whether or not the value class is responsible for freeing the payload
            bool should_free_payload = true;

        public:
            /* For arrays */
            explicit Value(ValueType type, std::vector<RuntimeValue*> *array);
            /* For strings */
            explicit Value(ValueType type, std::string* str);
            /* For numbers */
            explicit Value(ValueType type, number_t number);
            /* For program functions */
            explicit Value(Bytecode::constant_index_t prog_func_index, ValueType type);
            /* For native functions */
            explicit Value(native_method_t native);

            /* For literals: true, false, null */
            explicit Value(ValueType type);

            /* Default constructor so that Value can be part of arrays, hashmap, etc. */
            Value();

            friend std::string value_to_string(const Value &value);
            friend std::string value_to_debug_string(const Value &value);
            
            friend ValueType get_value_type(const Value &value);
            friend number_t get_value_number(const Value &value);
            friend std::string *get_value_string(const Value &value);
            friend std::vector<RuntimeValue*> *get_value_array(const Value &value);
            friend native_method_t get_value_native_function(const Value &value);
            friend Bytecode::constant_index_t get_value_program_function(const Value &value);

            inline void mark_payload() { this->should_free_payload = true; }
            void free_payload();

            number_t to_number() const;
    };

    std::string value_to_string(const Value &value);
    std::string value_to_debug_string(const Value &value);

    inline ValueType get_value_type(const Value &value) {
        return value.type;
    };
    inline number_t get_value_number(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::NUMBER);
        #endif
        return value.value.number;
    };
    inline std::string* get_value_string(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::STRING);
        #endif
        return value.value.str;
    }
    inline std::vector<RuntimeValue*>* get_value_array(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::ARRAY);
        #endif
        return value.value.array;
    }
    inline native_method_t get_value_native_function(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::NATIVE_FUNCTION);
        #endif
        return value.value.native;
    }
    inline Bytecode::constant_index_t get_value_program_function(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::PROGRAM_FUNCTION);
        #endif
        return value.value.prog_func_index;
    };

    bool value_is_truthy(const Value &value);
    bool value_is_numerical(const Value &value);
    bool values_are_equal(const Value &a, const Value &b);

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