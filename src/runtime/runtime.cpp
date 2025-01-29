#include "runtime.hpp"

Runtime::Runtime(Bytecode::Chunk &main) : main(main) {
    Natives::create_natives(this->natives);
};

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