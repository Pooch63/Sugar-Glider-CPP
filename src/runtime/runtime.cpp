#include "runtime.hpp"

#include <math.h>

RuntimeFunction::RuntimeFunction(Bytecode::Chunk chunk, Bytecode::call_arguments_t num_arguments) :
    chunk(chunk), num_arguments(num_arguments) {};

RuntimeCallFrame::RuntimeCallFrame(Bytecode::constant_index_t func_index, Bytecode::call_arguments_t arg_count, std::vector<Values::Value> &stack) :
    func_index(func_index), variables(std::vector<Values::Value>(arg_count)) {
        for (int var_ind = arg_count - 1; var_ind >= 0; var_ind -= 1) {
            variables[var_ind] = stack.back();
            stack.pop_back();
        }
    };

Runtime::Runtime(Bytecode::Chunk &main) : main(main) {
    Natives::create_natives(this->natives);
};

void Runtime::init_global_pool(size_t num_globals) {
    this->global_variables = std::vector<RuntimeValue*>(num_globals);
}

Bytecode::variable_index_t Runtime::new_constant(Values::Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
}
void Runtime::add_function(RuntimeFunction &func) {
    this->functions.push_back(func);
}

Bytecode::Chunk &Runtime::get_running_block() {
    if (this->call_stack.size() > 0) {
        return this->functions.at(this->call_stack.back().func_index).chunk;
    }
    else return this->main;
};

using namespace Values;
using namespace Bytecode;

RuntimeValue::RuntimeValue(Values::Value &value, RuntimeValue *next) :
    value(value),  marked_to_save(false), next(next) {}

RuntimeValue *Runtime::get_runtime_value(Values::Value &value) {
    try {
        RuntimeValue *runtime_value = new RuntimeValue(value, this->runtime_values);
        this->runtime_values = runtime_value;
        return runtime_value;
    } catch (std::runtime_error &err) {
        this->error = "Sugar Glider Runtime ran out of memory. Heap allocation failed.";
        return nullptr;
    }
};

RuntimeValue::~RuntimeValue() {
    this->value.free_payload();
}

void Runtime::push_stack_value(Values::Value value) {
    switch (get_value_type(value)) {
        case ValueType::STRING:
            this->get_runtime_value(value);
            this->stack.push_back(value);
            break;
        // Any other value doesn't need to be allocated
        default:
            this->stack.push_back(value);
    }
};

void Runtime::mark_value(RuntimeValue *value) {
    value->marked_to_save = true;
};
void Runtime::mark_values() {
    // Mark every value referenced by variables

    // Unmark everything. We're only saving the values we need to
    for (RuntimeValue *value : this->global_variables) {
        value->marked_to_save = false;
    }

    // First, globals
    for (RuntimeValue *value : this->global_variables) {
        mark_value(value);
        std::cout << "marked value wtf?? " << value_to_debug_string(value->value) << "\n";
    }
}
void Runtime::delete_values() {
    RuntimeValue *current = this->runtime_values;
    this->runtime_values = nullptr;

    while (true) {
        RuntimeValue *next = current->next;

        if (!current->marked_to_save) {
            std::cout << "freeing " << current << '\n';
            delete current;
            std::cout << "freed " << '\n';
        }
        else {
            current->next = this->runtime_values;
            this->runtime_values = current;
        }

        if (next == nullptr) break;
        current = next;
    }
}
void Runtime::run_gc() {
    #ifdef DEBUG
    assert(this->stack.size() == 0 &&
        "as of now, the GC cannot be run when the stack has an element, because "
        "we might accidentally free a string referenced in the stack");
    #endif

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
    assert(!this->global_variables.empty() && "Runtime global variable pool must be initialized before running");
    #endif

    main_ip = 0;

    while (this->call_stack.size() > 0 || this->main_ip < this->main.code_byte_count()) {
        Bytecode::address_t &prog_ip = 
            this->call_stack.size() > 0 ? this->call_stack.back().ip : this->main_ip;

        OpCode code = this->get_running_block().read_opcode(prog_ip);

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
                    native.func(this->stack.data(), this->stack.size(), result, this->error);

                    // Pop arguments
                    for (int pop = 0; pop < native.number_arguments; pop += 1) {
                        this->stack.pop_back();
                    }

                    this->push_stack_value(result);
                }
                else if (get_value_type(func) == Values::PROGRAM_FUNCTION) {
                    Bytecode::constant_index_t func_ind = get_value_program_function(func);

                    this->call_stack.push_back(
                        RuntimeCallFrame(func_ind, this->functions.at(func_ind).num_arguments, this->stack)
                    );

                    uint stack_size = this->functions.at(func_ind).num_arguments * sizeof(Value) + sizeof(RuntimeCallFrame);
                    this->call_stack_size += stack_size;

                    if (this->call_stack_size > MAX_CALL_STACK_SIZE) {
                        this->error = "Maximum call stack size exceeded. ";
                        this->error = std::to_string(this->call_stack_size / 1024.0);
                        this->error += " KB necessary, but maximum is ";
                        this->error += std::to_string(MAX_CALL_STACK_SIZE);
                        this->error += " KB";
                        break;
                    }

                    this->call_stack_size -= stack_size;
                }
                else {
                    this->error = "Cannot call non-function value ";
                    this->error += value_to_string(func);
                    break;
                }
            }
                break;

            case OpCode::OP_POP:
            {
                this->stack_pop();
            }
                break;
            case OpCode::OP_GOTO:
            {
                address_t address = this->get_running_block().read_address(prog_ip);
                prog_ip = address;
            }
                break;
            case OpCode::OP_POP_JIZ:
            {
                address_t address = this->get_running_block().read_address(prog_ip);
                Value condition = this->stack_pop();
                if (!Values::value_is_truthy(condition)) prog_ip = address;
            }
                break;
            case OpCode::OP_POP_JNZ:
            {
                address_t address = this->get_running_block().read_address(prog_ip);
                Value condition = this->stack_pop();
                if (Values::value_is_truthy(condition)) prog_ip = address;
            }
                break;

            case OpCode::OP_STORE_GLOBAL:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                Value value = this->stack_pop();
                this->global_variables[index] = this->get_runtime_value(value);
            }
                break;
            case OpCode::OP_LOAD_GLOBAL:
            {
                variable_index_t index = this->read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->global_variables[index]->value);
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
            case OpCode::OP_MAKE_ARRAY: {
                variable_index_t element_count = this->read_value<variable_index_t>(prog_ip);
                std::vector<Value> *array = new std::vector<Value>();

                // Add the elements to the array
                for (uint value_index = this->stack.size() - element_count; value_index < this->stack.size(); value_index += 1) {
                    array->push_back(this->stack.at(value_index));
                }
                // Now pop the results from the stack
                for (uint pop = 0; pop < element_count; pop += 1) {
                    this->stack.pop_back();
                }

                this->stack.push_back( Values::Value(ValueType::ARRAY, array) );
            }
                break;
            // Automatically push a copy of the push value if we're setting a value, e.g. arr[ind] = 3;
            case OpCode::OP_GET_ARRAY_VALUE:
            case OpCode::OP_SET_ARRAY_VALUE: {
                Values::Value set_value;
                if (code == OpCode::OP_SET_ARRAY_VALUE) {
                    set_value = this->stack_pop();
                }

                Values::Value index_value = this->stack_pop();
                Values::Value array_value = this->stack_pop();

                if (get_value_type(index_value) != ValueType::NUMBER) {
                    this->error = "Array index must be a number, but given index ";
                    this->error += value_to_string(index_value);
                    break;
                }
                if (get_value_type(array_value) != ValueType::ARRAY) {
                    this->error = "Cannot index non-array value ";
                    this->error += value_to_string(array_value);
                    break;
                }

                Values::number_t index = get_value_number(index_value);
                std::vector<Value> *array = get_value_array(array_value);
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
                break;
            
            case OpCode::OP_BIN:
            {
                Operations::BinOpType type = this->get_running_block().read_small_enum<Operations::BinOpType>(prog_ip);
                Value b = this->stack_pop();
                Value a = this->stack_pop();
                Value result;
                bool valid = Values::bin_op(type, a, b, &result, &this->error);

                if (!valid) break;
                this->push_stack_value(result);
            }
                break;
            case OpCode::OP_UNARY:
            {
                Operations::UnaryOpType type = this->get_running_block().read_small_enum<Operations::UnaryOpType>(prog_ip);
                Value arg = this->stack_pop();
                Value result;
                bool valid = Values::unary_op(type, arg, &result, &this->error);

                if (!valid) break;
                this->push_stack_value(result);
            }
                break;

            case OpCode::OP_RETURN:
                this->call_stack.pop_back();
                break;

            case OpCode::OP_EXIT:
                this->exit();
                return 0;
            default: std::cerr << "unhandled " << instruction_to_string(code) << std::endl; break;
        }
        if (this->error.size() > 0) {
            std::cerr << rang::fg::red << "runtime this->error: " << rang::style::reset << this->error << std::endl;
            return -1;
        }
    }

    throw std::runtime_error("Reached end of runtime loop, but the exit command completely returns. Logic error");
}

Runtime::~Runtime() {
    // for (Value value : this->constants) {
    //     value.free_payload();
    // }

    while (this->runtime_values != nullptr) {
        RuntimeValue *next = this->runtime_values->next;
        delete this->runtime_values;
        this->runtime_values = next;
    }
}
