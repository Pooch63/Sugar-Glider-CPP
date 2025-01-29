#include "runtime.hpp"

Runtime::Runtime(Bytecode::Chunk &main) : main(main) {
    Natives::create_natives(this->natives);
};
void Runtime::init_global_pool(size_t num_globals) {
    // Make sure we haven't already initialized the variable pool
    #ifdef DEBUG
    assert(this->global_variables == nullptr);
    #endif
    this->global_variables = new Values::Value[num_globals];
}

Bytecode::variable_index_t Runtime::new_constant(Values::Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
}

using Values::Value;
using namespace Bytecode;

Values::Value Runtime::stack_pop() {
    Values::Value back = this->stack.back();
    this->stack.pop_back();
    return back;
}
void Runtime::exit() {}

int Runtime::run() {
    #ifdef DEBUG
    assert(this->global_variables != nullptr && "Runtime global variable pool must be initialized before running");
    #endif

    address_t ip = 0;

    std::string error = "";

    while (ip < this->main.code_byte_count()) {
        OpCode code = this->main.read_opcode(ip);

        switch (code) {
            case OpCode::OP_LOAD_CONST:
            {
                constant_index_t index = this->main.read_value<constant_index_t>(ip);
                this->stack.push_back(this->constants.at(index));
            }
                break;
            case OpCode::OP_LOAD_NATIVE:
            {
                variable_index_t index = this->main.read_value<variable_index_t>(ip);
                this->stack.push_back(this->natives.at(index));
            }
                break;

            case OpCode::OP_CALL:
            {
                Value func = this->stack_pop();
                if (func.get_type() != Values::NATIVE_FUNCTION) {
                    error = "Cannot call non-function value";
                    error += func.to_debug_string();
                    break;
                }
                Values::native_method_t native = func.get_native_function();
                call_arguments_t num_args = this->main.read_value<call_arguments_t>(ip);
                if (num_args != native.number_arguments) {
                    error = std::to_string(num_args);
                    error += " argument(s) passed to function expecting ";
                    error += std::to_string(native.number_arguments);
                    break;
                }

                Values::Value result;
                native.func(this->stack.data(), this->stack.size(), result, error);
            }
                break;

            case OpCode::OP_POP:
            {
                this->stack_pop();
            }
                break;
            case OpCode::OP_GOTO:
            {
                address_t address = this->main.read_address(ip);
                ip = address;
            }
                break;

            case OpCode::OP_STORE_GLOBAL:
            {
                variable_index_t index = this->main.read_value<variable_index_t>(ip);
                Value value = this->stack_pop();
                this->global_variables[index] = value;
            }
                break;
            case OpCode::OP_LOAD_GLOBAL:
            {
                variable_index_t index = this->main.read_value<variable_index_t>(ip);
                this->stack.push_back(this->global_variables[index]);
            }
                break;

            case OpCode::OP_TRUE: this->stack.push_back(Value(Values::TRUE)); break;
            case OpCode::OP_FALSE: this->stack.push_back(Value(Values::FALSE)); break;
            case OpCode::OP_NULL: this->stack.push_back(Value(Values::NULL_VALUE)); break;
            
            case OpCode::OP_BIN:
            {
                Operations::BinOpType type = this->main.read_small_enum<Operations::BinOpType>(ip);
                Value b = this->stack_pop();
                Value a = this->stack_pop();
                Value result;
                bool valid = Values::bin_op(type, a, b, &result, &error);

                if (!valid) break;

                this->stack.push_back(result);
            }
                break;

            case OpCode::OP_EXIT:
                this->exit();
                break;
            default: break;
        }
        if (error.size() > 0) {
            std::cout << rang::fg::red << "runtime error: " << rang::style::reset << error << std::endl;
            return -1;
        }
    }

    return 0;
}

Runtime::~Runtime() {
    for (Value value : this->constants) {
        value.free_payload();
    }

    delete [] this->global_variables;
}