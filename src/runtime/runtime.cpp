#include "runtime.hpp"

#include <array>
#include <unordered_map>

#include <math.h>

using namespace Values;
using namespace Bytecode;

RuntimeFunction::RuntimeFunction(
    Bytecode::Chunk chunk,
    Bytecode::call_arguments_t num_arguments,
    Bytecode::variable_index_t total_variables,
    const std::string &name) :
    chunk(chunk), num_arguments(num_arguments), total_variables(total_variables), name(name) {};
RuntimeCallFrame::RuntimeCallFrame(
    Bytecode::constant_index_t func_index,
    Bytecode::call_arguments_t arg_count,
    std::vector<Values::Value> &stack,
    std::vector<Values::Value>::iterator variables_start) :
    func_index(func_index), variables_start(variables_start) {
        for (int var_ind = arg_count - 1; var_ind >= 0; var_ind -= 1) {
            variables_start[var_ind] = stack.back();
            stack.pop_back();
        }
    };

Runtime::Runtime(Bytecode::Chunk &main) : main(main) {
    Natives::create_natives(this->natives);
    this->running_blocks.push_back(&this->main);
};

void Runtime::init_global_pool(size_t num_globals) {
    this->global_variables = std::vector<Value>(num_globals);
    this->variable_stack_size = num_globals;
}
Bytecode::variable_index_t Runtime::new_constant(Values::Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
}
void Runtime::add_function(RuntimeFunction &func) {
    this->functions.push_back(func);
}


void Runtime::add_object(Object *obj) {
    #ifdef DEBUG_STRESS_GC
        std::cout << "GC: Allocating value (" << obj << ") " <<
            object_to_debug_string(obj) << " on heap\n";
        this->run_gc();
    #endif

    this->gc_size += sizeof(Object*);
    switch (obj->type) {
        case ObjectType::ARRAY:
            this->gc_size += sizeof(obj_mem_t::array) + sizeof(*obj_mem_t::array);
            break;
        case ObjectType::STRING:
            this->gc_size += sizeof(obj_mem_t::str) + sizeof(*obj_mem_t::str) + obj->memory.str->size();;
            break;
        // Constant namespaces are allocated at compile time
        case ObjectType::NAMESPACE_CONSTANT: throw sg_assert_error("Tried to allocate at runtime a compile-time constant namespace");
    }

    #ifdef DEBUG_GC
    std::cout << "gc_size=" << this->gc_size << std::endl;
    #endif

    obj->next = this->runtime_values;
    this->runtime_values = obj;
};

void Runtime::push_stack_value(Values::Value value) {
    this->stack.push_back(value);
};

void Runtime::mark_object(Values::Value value) {
    Object *obj = safe_get_value_object(value);
    if (obj == nullptr) return;

    #ifdef DEBUG_GC
    std::cout << "GC: Marking value " << value_to_debug_string(value) <<
        " at " << get_value_object(value) << "\n";
    #endif

    obj->marked_for_save = true;

    // If it's array, get all of ITS values as well
    if (obj->type == ObjectType::ARRAY) {
        for (Values::Value &value : *get_value_array(value)) {
            mark_object(value);
        }
    }
};
void Runtime::mark_values() {
    // Mark every value referenced by variables

    // Unmark everything. We're only saving the values we need to
    Object *obj = this->runtime_values;
    while (obj != nullptr) {
        // When the global pool is initialized, all variable slots are set to nullptr,
        // so don't mark those
        #ifdef DEBUG_GC
        std::cout << "GC: Unmarking value " << object_to_debug_string(obj) <<
            " at " << obj << '\n';
        #endif
        obj->marked_for_save = false;

        obj = obj->next;
    }

    // Now mark
    // First, globals
    for (Values::Value &value : this->global_variables) {
        mark_object(value);
    }

    // Next, everything on the stack
    for (Values::Value &value : this->stack) {
        std::cout << "GC: Moving to mark stack value " << value_to_debug_string(value) << std::endl;
        mark_object(value);
    }
}
void Runtime::delete_values() {
    Object *current = this->runtime_values;
    this->runtime_values = nullptr;

    while (current != nullptr) {
        Object *next = current->next;
        
        // Note: we can't log any information about objects that are being deleted.
        // This is because they might depend on other objects that were previously deleted
        // E.g., if [ "a" ] is dereferenced, we might delete "a" first. Thus, the array itself cannot
        // be logged
        if (!current->marked_for_save) {
            #ifdef DEBUG_GC
            std::cout << "GC: Deleting value @ " << current << std::endl;
            #endif

            this->gc_size -= sizeof(Object) + sizeof(Object*);
            delete current;
        }
        else {
            #ifdef DEBUG_GC
            std::cout << "GC: Saving value (" << current << ") " << object_to_debug_string(current) << std::endl;
            #endif

            current->next = this->runtime_values;
            this->runtime_values = current;
        }

        current = next;
    }
}
void Runtime::run_gc() {
    this->mark_values();
    this->delete_values();
}

Values::Value Runtime::stack_pop() {
    #ifdef DEBUG
    assert(this->stack.size() > 0);
    #endif

    Values::Value back = this->stack.back();
    this->stack.pop_back();
    return back;
}

void Runtime::log_call_frame(RuntimeCallFrame &frame, std::ostream &out) {
    RuntimeFunction func = this->functions.at(frame.func_index);
    out << func.name << "(...)" << std::endl;
}
void Runtime::log_stack_trace(std::ostream &out) {
    if (this->call_stack.size() == 0) return;

    int bottom_ind = std::max(0, static_cast<int>(this->call_stack.size()) - 3);
    // Log first 3
    for (int ind = this->call_stack.size() - 1;
        ind >= bottom_ind;
        ind -= 1
    ) {
        log_call_frame(this->call_stack.at(ind), out);
    }
    if (this->call_stack.size() > 6) {
        std::cout << "...\n";
    }
    for (int ind = std::min(static_cast<int>(2), bottom_ind); ind >= 0; ind -= 1) {
        log_call_frame(this->call_stack.at(ind), out);
    }
}
void Runtime::exit() {}

void Runtime::log_instructions() {
    this->main.print_code(this);

    for (size_t func_ind = 0; func_ind < this->functions.size(); func_ind += 1) {
        std::cout << "-------------------------------------------------------\n";
        std::cout << '\n' << rang::fg::green;
        std::cout << "                       Function 0x" << std::hex << func_ind << std::dec << rang::style::reset << '\n';

        this->functions.at(func_ind).chunk.print_code(this);
    }
};

int Runtime::run() {
    #ifdef DEBUG
    assert("Runtime global variable pool must be initialized before running");
    #endif

    main_ip = 0;

    while (this->call_stack.size() > 0 || this->main_ip < this->main.code_byte_count()) {
        Bytecode::address_t &prog_ip = 
            this->call_stack.size() > 0 ? this->call_stack.back().ip : this->main_ip;

        OpCode code = this->get_running_block()->read_opcode(prog_ip);

        switch (code) {
            case OpCode::OP_LOAD_CONST:
            {
                constant_index_t index = this->read_value<constant_index_t>(prog_ip);
                this->stack.push_back(this->constants.at(index));
            }
                break;
            case OpCode::OP_LOAD_NATIVE:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->natives.at(index));
            }
                break;

            case OpCode::OP_CALL:
            {
                Value func = this->stack_pop();
                call_arguments_t num_args = this->read_value<call_arguments_t>(prog_ip);

                if (get_value_type(func) == Values::NATIVE_FUNCTION) {
                    Values::native_method_t native = get_value_native_function(func);
                    if (num_args != native.number_arguments) {
                        this->error = std::to_string(num_args);
                        this->error += " argument(s) passed to function expecting ";
                        this->error += std::to_string(native.number_arguments);
                        break;
                    }

                    Values::Value result;
                    native.func(
                        (this->stack.begin() + (this->stack.size() - num_args)).base(),
                        this->stack.size(),
                        result, *this, this->error);

                    // Pop arguments
                    for (int pop = 0; pop < native.number_arguments; pop += 1) {
                        this->stack.pop_back();
                    }

                    this->push_stack_value(result);
                }
                else if (get_value_type(func) == Values::PROGRAM_FUNCTION) {
                    Bytecode::constant_index_t func_ind = get_value_program_function(func);
                    Bytecode::call_arguments_t num_args = this->functions.at(func_ind).num_arguments;

                    Bytecode::variable_index_t total_variables = this->functions.at(func_ind).total_variables;
                    size_t necessary_space = this->variable_stack_size + total_variables;
                    
                    if (this->global_variables.size() < necessary_space) {
                        this->global_variables.resize(necessary_space);
                    }
                    this->call_stack.push_back(
                        RuntimeCallFrame(func_ind, num_args, this->stack, this->global_variables.begin() + this->variable_stack_size)
                    );
                    this->variable_stack_size += total_variables;
                    this->running_blocks.push_back(&this->functions.at(func_ind).chunk);

                    uint stack_size = total_variables * sizeof(Value) + sizeof(RuntimeCallFrame);
                    this->call_stack_size += stack_size;

                    if (this->call_stack_size > MAX_CALL_STACK_SIZE) {
                        this->error = "Stack error: Maximum call stack size exceeded. ";
                        double size = this->call_stack_size / 1024.0;
                        char num[20];
                        snprintf(num, 20, "%.2lf", size);
                        this->error += num;
                        this->error += " KB necessary, but maximum is ";
                        this->error += std::to_string(MAX_CALL_STACK_SIZE / 1024);
                        this->error += " KB";
                        break;
                    }
                }
                else {
                    this->error = "Cannot call non-function value ";
                    this->error += value_to_string(func);
                    break;
                }
            }
                break;

            case OpCode::OP_RETURN:
            {
                uint total_variables = this->functions.at(this->call_stack.back().func_index).total_variables;
                this->call_stack_size -= total_variables * sizeof(Value) + sizeof(RuntimeCallFrame);
                this->variable_stack_size -= total_variables;
                this->call_stack.pop_back();
                this->running_blocks.pop_back();
            }
                break;

            case OpCode::OP_POP:
            {
                this->stack_pop();
            }
                break;
            case OpCode::OP_GOTO:
            {
                address_t address = this->get_running_block()->read_address(prog_ip);
                prog_ip = address;
            }
                break;
            case OpCode::OP_POP_JIZ:
            {
                address_t address = this->get_running_block()->read_address(prog_ip);
                Value condition = this->stack_pop();
                if (!Values::value_is_truthy(condition)) prog_ip = address;
            }
                break;
            case OpCode::OP_POP_JNZ:
            {
                address_t address = this->get_running_block()->read_address(prog_ip);
                Value condition = this->stack_pop();
                if (Values::value_is_truthy(condition)) prog_ip = address;
            }
                break;

            case OpCode::OP_STORE_GLOBAL:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                Value value = this->stack_pop();
                this->global_variables[index] = value;
            }
                break;
            case OpCode::OP_LOAD_GLOBAL:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->global_variables[index]);
            }
                break;

            case OpCode::OP_STORE_FRAME_VAR:
            {
                Value value = this->stack_pop();
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                this->call_stack.back().set_variable(index, value);
            }
                break;
            case OpCode::OP_LOAD_FRAME_VAR:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->call_stack.back().get_variable(index));
            }
                break;

            case OpCode::OP_TRUE: this->push_stack_value(Value(Values::TRUE)); break;
            case OpCode::OP_FALSE: this->push_stack_value(Value(Values::FALSE)); break;
            case OpCode::OP_NULL: this->push_stack_value(Value(Values::NULL_VALUE)); break;
            case OpCode::OP_MAKE_ARRAY:
            {
                variable_index_t element_count = this->read_value<variable_index_t>(prog_ip);
                std::vector<Value> *array = this->create<std::vector<Value>>(element_count);

                // Add the elements to the array
                for (uint value_index = this->stack.size() - element_count; value_index < this->stack.size(); value_index += 1) {
                    (*array)[value_index] = this->stack.at(value_index);
                }
                // Now pop the results from the stack
                for (uint pop = 0; pop < element_count; pop += 1) {
                    this->stack.pop_back();
                }

                Object *obj = this->create<Object>(array);
                this->add_object(obj);

                this->stack.push_back(Values::Value(obj));
            }
                break;
            // Automatically push a copy of the push value if we're setting a value, e.g. arr[ind] = 3;
            case OpCode::OP_GET_ARRAY_VALUE:
            case OpCode::OP_SET_ARRAY_VALUE:
            {
                Values::Value set_value;
                if (code == OpCode::OP_SET_ARRAY_VALUE) {
                    set_value = this->stack_pop();
                }

                Values::Value index_value = this->stack_pop();
                Values::Value array_value = this->stack_pop();
                if (get_value_type(index_value) != ValueType::NUMBER) {
                    this->error = "Index must be a number, but given index ";
                    this->error += value_to_string(index_value);
                    break;
                }

                Object *array_obj = safe_get_value_object(array_value);;
                if (
                    array_obj == nullptr ||
                    (array_obj->type != ObjectType::ARRAY && array_obj->type != ObjectType::STRING)
                ) {
                    this->error = "Cannot index value ";
                    this->error += value_to_string(array_value);
                    break;
                }

                Values::number_t index = get_value_number(index_value);

                if (array_obj->type == ObjectType::ARRAY) {
                    std::vector<Value> *array = array_obj->memory.array;
                    if (index > static_cast<Values::number_t>(array->size()) || index < 0 || index != floor(index)) {
                        this->error = "Array index must be an integer within the range of array's values, but index was ";
                        this->error += value_to_string(index_value);
                        break;
                    }

                    if (code == OpCode::OP_GET_ARRAY_VALUE) {
                        this->stack.push_back(array->at(floor(index)));
                    }
                    else {
                        (*array)[static_cast<uint>(floor(index))] = set_value;
                        this->stack.push_back(set_value);
                    }
                }
                else if (array_obj->type == ObjectType::STRING) {
                    if (code == OpCode::OP_SET_ARRAY_VALUE) {
                        this->error = "Strings are immutable. Cannot update string ";
                        this->error += value_to_string(array_value);
                    }
                    std::string *str = array_obj->memory.str;
                    if (index > static_cast<Values::number_t>(str->size()) || index < 0 || index != floor(index)) {
                        this->error = "String index must be an integer within the range of array's values, but index was ";
                        this->error += value_to_string(index_value);
                        break;
                    }

                    std::string *character = this->create<std::string>(1, (*str)[index]);
                    Object *obj = this->create<Object>(character);
                    this->add_object(obj);
                    this->push_stack_value(Value(obj));
                    break;
                }
            }
                break;

            case OpCode::OP_CONSTANT_PROPERTY_ACCESS:
            {
                Value left = this->stack_pop();
                Object *obj = safe_get_value_object(left);

                std::string *property_name = this->read_value<std::string*>(prog_ip);

                if (obj == nullptr || obj->type != ObjectType::NAMESPACE_CONSTANT) {
                    this->error = "Cannot access property ";
                    this->error += *property_name;
                    this->error += " of non-object value ";
                    this->error += value_to_string(left);
                    break;
                }

                auto namespace_ = obj->memory.namespace_;
                auto property = namespace_->find(*property_name);
                if (property == namespace_->end()) {
                    this->push_stack_value(Value(ValueType::NULL_VALUE));
                }
                else {
                    this->push_stack_value(property->second);
                }
            }
                break;
            
            case OpCode::OP_BIN:
            {
                Operations::BinOpType type = this->get_running_block()->read_small_enum<Operations::BinOpType>(prog_ip);
                Value b = this->stack_pop();
                Value a = this->stack_pop();
                Value result;
                bool valid = Values::bin_op(type, a, b, &result, &this->error);

                if (!valid) break;

                /* Make sure that we add the object to the GC if necessary */
                if (get_value_type(result) == ValueType::OBJ) {
                    this->add_object(get_value_object(result));
                }

                this->push_stack_value(result);
            }
                break;
            case OpCode::OP_UNARY:
            {
                Operations::UnaryOpType type = this->get_running_block()->read_small_enum<Operations::UnaryOpType>(prog_ip);
                Value arg = this->stack_pop();
                Value result;
                bool valid = Values::unary_op(type, arg, &result, &this->error);

                if (!valid) break;
                this->push_stack_value(result);
            }
                break;

            case OpCode::OP_EXIT:
                this->exit();
                return 0;
            default: std::cerr << "unhandled " << instruction_to_string(code) << std::endl; break;
        }
        if (this->error.size() > 0) {
            std::cerr << rang::fg::red << "runtime error: " << rang::style::reset << this->error << std::endl;
            
            this->log_stack_trace(std::cerr);
            
            return -1;
        }
    }

    throw std::runtime_error("Reached end of runtime loop, but the exit command completely returns. Logic error");
}

Runtime::~Runtime() {
    for (Value value : this->natives) {
        free_value_if_object(value);
    }
    for (Value value : this->constants) {
        free_value_if_object(value);
    }

    while (this->runtime_values != nullptr) {
        Object *next = this->runtime_values->next;
        delete this->runtime_values;
        this->runtime_values = next;
    }
}
