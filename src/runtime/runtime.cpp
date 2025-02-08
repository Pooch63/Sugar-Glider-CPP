#include "runtime.hpp"

RuntimeFunction::RuntimeFunction(Bytecode::Chunk chunk, Bytecode::call_arguments_t num_arguments) :
    chunk(chunk), num_arguments(num_arguments) {};

RuntimeCallFrame::RuntimeCallFrame(Bytecode::constant_index_t func_index, Bytecode::call_arguments_t arg_count) :
    func_index(func_index), variables(std::vector<Values::Value>(arg_count)) {};

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
    assert(this->global_variables != nullptr && "Runtime global variable pool must be initialized before running");
    #endif

    main_ip = 0;
    std::string error = "";

    while (this->call_stack.size() > 0 || this->main_ip < this->main.code_byte_count()) {
        Bytecode::address_t &prog_ip = 
            this->call_stack.size() > 0 ? this->call_stack.back().ip : this->main_ip;

        OpCode code = this->get_running_block().read_opcode(prog_ip);

        switch (code) {
            case OpCode::OP_LOAD_CONST:
            {
                constant_index_t index = this->get_running_block().read_value<constant_index_t>(prog_ip);
                this->stack.push_back(this->constants.at(index));
            }
                break;
            case OpCode::OP_LOAD_NATIVE:
            {
                variable_index_t index = this->get_running_block().read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->natives.at(index));
            }
                break;

            case OpCode::OP_CALL:
            {
                Value func = this->stack_pop();
                call_arguments_t num_args = this->get_running_block().read_value<call_arguments_t>(prog_ip);

                if (get_value_type(func) == Values::NATIVE_FUNCTION) {
                    Values::native_method_t native = get_value_native_function(func);
                    if (num_args != native.number_arguments) {
                        error = std::to_string(num_args);
                        error += " argument(s) passed to function expecting ";
                        error += std::to_string(native.number_arguments);
                        break;
                    }

                    Values::Value result;
                    native.func(this->stack.data(), this->stack.size(), result, error);
                    this->stack.push_back(result);
                }
                else if (get_value_type(func) == Values::PROGRAM_FUNCTION) {
                    Bytecode::constant_index_t func_ind = get_value_program_function(func);

                    this->call_stack.push_back(
                        RuntimeCallFrame(func_ind, this->functions.at(func_ind).num_arguments)
                    );

                    uint stack_size = this->functions.at(func_ind).num_arguments * sizeof(Value) + sizeof(RuntimeCallFrame::variables);
                    this->call_stack_size += stack_size;

                    if (this->call_stack_size > MAX_CALL_STACK_SIZE) {
                        error = "Maximum call stack size exceeded. ";
                        error = std::to_string(this->call_stack_size / 1024.0);
                        error += " KB necessary, but maximum is ";
                        error += std::to_string(MAX_CALL_STACK_SIZE);
                        error += " KB";
                        break;
                    }

                    this->call_stack_size -= stack_size;
                }
                else {
                    error = "Cannot call non-function value ";
                    error += value_to_string(func);
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
                variable_index_t index = this->get_running_block().read_value<variable_index_t>(prog_ip);
                Value value = this->stack_pop();
                this->global_variables[index] = value;
            }
                break;
            case OpCode::OP_LOAD_GLOBAL:
            {
                variable_index_t index = this->get_running_block().read_value<variable_index_t>(prog_ip);
                this->stack.push_back(this->global_variables[index]);
            }
                break;

            case OpCode::OP_TRUE: this->stack.push_back(Value(Values::TRUE)); break;
            case OpCode::OP_FALSE: this->stack.push_back(Value(Values::FALSE)); break;
            case OpCode::OP_NULL: this->stack.push_back(Value(Values::NULL_VALUE)); break;
            
            case OpCode::OP_BIN:
            {
                Operations::BinOpType type = this->get_running_block().read_small_enum<Operations::BinOpType>(prog_ip);
                Value b = this->stack_pop();
                Value a = this->stack_pop();
                Value result;
                bool valid = Values::bin_op(type, a, b, &result, &error);

                if (!valid) break;
                this->stack.push_back(result);
            }
                break;
            case OpCode::OP_UNARY:
            {
                Operations::UnaryOpType type = this->get_running_block().read_small_enum<Operations::UnaryOpType>(prog_ip);
                Value arg = this->stack_pop();
                Value result;
                bool valid = Values::unary_op(type, arg, &result, &error);

                if (!valid) break;
                this->stack.push_back(result);
            }
                break;

            case OpCode::OP_RETURN:
                this->call_stack.pop_back();
                break;

            case OpCode::OP_EXIT:
                this->exit();
                break;
            default: std::cout << "unhandled " << instruction_to_string(code) << std::endl; break;
        }
        if (error.size() > 0) {
            std::cerr << rang::fg::red << "runtime error: " << rang::style::reset << error << std::endl;
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
