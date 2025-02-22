#ifndef _SGCPP_VALUE_HPP
#define _SGCPP_VALUE_HPP

#include "globals.hpp"
#include "ir/bytecode.hpp"
#include "operations.hpp"

#ifdef DEBUG
// For inline value functions
#include <cassert>
#endif

#include <unordered_map>

class Runtime;

namespace Values {
    class Value;

    enum ValueType {
        OBJ,
        NUMBER,
        TRUE,
        FALSE,
        NULL_VALUE,

        PROGRAM_FUNCTION,
        NATIVE_FUNCTION
    };
    /**
     * @param {Value*} - start of argument stack
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
    class Object;
    
    /* Objects are responsible for the container and ont the elements inside.
        E.g., arrays aren't responsible for the elements themselves, and
        namespaces are NOT responsible for the values. */
    union value_mem_t {
        number_t number;
        native_method_t native;
        Bytecode::constant_index_t prog_func_index;
        Object *obj;
    };

    class Value {
        private:
            ValueType type;
            value_mem_t value;

        public:
            /* For arrays or strings */
            explicit Value(Object *obj);
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
            friend std::vector<Value> *get_value_array(const Value &value);
            friend native_method_t get_value_native_function(const Value &value);
            friend Bytecode::constant_index_t get_value_program_function(const Value &value);
            friend Object *get_value_object(const Value &value);

            number_t to_number() const;
    };

    typedef std::unordered_map<std::string, Value> namespace_t;
    union obj_mem_t {
        std::string *str;
        std::vector<Value> *array;
        namespace_t *namespace_;
    };
    enum ObjectType {
        STRING,
        ARRAY,
        // A namespace that cannot be updated
        NAMESPACE_CONSTANT
    };
    // For values that need to be allocated on the heap
    // The runtime itself will add the next linked list value, so
    // you can't pass it in the constructor
    struct Object {
        ObjectType type;
        obj_mem_t memory;
        bool marked_for_save = false;
        Object *next;

        Object(std::string *str);
        Object(std::vector<Value> *str);
        Object(namespace_t *namespace_);

        ~Object();
    };

    std::string value_to_string(const Value &value);
    std::string value_to_debug_string(const Value &value);
    std::string object_to_debug_string(Object *obj);

    // Free value payload if necessary
    void free_value_if_object(Value &value);

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
        assert(get_value_type(value) == ValueType::OBJ &&
            get_value_object(value)->type == ObjectType::STRING);
        #endif
        return value.value.obj->memory.str;
    }
    inline std::vector<Value>* get_value_array(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::OBJ &&
            get_value_object(value)->type == ObjectType::ARRAY);
        #endif
        return value.value.obj->memory.array;
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
    inline Object *get_value_object(const Value &value) {
        #ifdef DEBUG
        assert(get_value_type(value) == ValueType::OBJ);
        #endif
        return value.value.obj;
    };
    // Returns nullptr if the value is not an object
    Object *safe_get_value_object(const Value &value);

    bool value_is_truthy(const Value &value);
    bool value_is_numerical(const Value &value);
    bool values_are_equal(const Value &a, const Value &b);

    /**
     * @param {BinOpType} type
     * @param {Value} a
     * @param {Value} b
     * @param {Value*} result - The place to add the result
     * @return {bool} - True if ok, false if error
     */
    bool bin_op(
        Operations::BinOpType type,
        Value a,
        Value b,
        Value *result,
        std::string *error);
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