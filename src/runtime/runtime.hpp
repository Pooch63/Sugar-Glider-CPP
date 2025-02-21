#ifndef _SG_CPP_RUNTIME_HPP
#define _SG_CPP_RUNTIME_HPP

#include "natives.hpp"
#include "../ir/bytecode.hpp"
#include "../value.hpp"

#include <array>
#include <vector>

struct RuntimeFunction {
    Bytecode::Chunk chunk;
    Bytecode::call_arguments_t num_arguments;

    RuntimeFunction(Bytecode::Chunk chunk, Bytecode::call_arguments_t num_arguments);
};
struct RuntimeCallFrame {
    Bytecode::constant_index_t func_index;
    std::vector<Values::Value> variables;
    uint ip = 0;

    inline Values::Value get_variable(Bytecode::variable_index_t index) const { return this->variables.at(index); };

    // Pops the function variable values off the stack
    RuntimeCallFrame(Bytecode::constant_index_t func_index, Bytecode::call_arguments_t arg_count, std::vector<Values::Value> &stack);
};

class Runtime {
    private:
        Bytecode::Chunk main = Bytecode::Chunk();

        std::vector<RuntimeFunction> functions = std::vector<RuntimeFunction>();
        /* Make a separate copy for natives so that each individual runtime can update native namespaces */
        std::array<Values::Value, Natives::native_count> natives = std::array<Values::Value, Natives::native_count>();
        Values::Object *runtime_values = nullptr;

        std::vector<Values::Value> stack = std::vector<Values::Value>();
        // Add value to stack that needs to be allocated at runtime
        void push_stack_value(Values::Value value);
        Values::Value stack_pop();

        std::vector<Values::Value> constants = std::vector<Values::Value>();
        std::vector<Values::Value> global_variables;

        std::vector<Bytecode::Chunk*> running_blocks = std::vector<Bytecode::Chunk*>();
        std::vector<RuntimeCallFrame> call_stack = std::vector<RuntimeCallFrame>();
        size_t call_stack_size = 0;

        inline Bytecode::Chunk *get_running_block() const {
            return this->running_blocks.back();
        };

        template<typename read_type>
        inline read_type read_value(Bytecode::address_t &prog_ip) {
            return this->get_running_block()->read_value<read_type>(prog_ip);
        }
        Bytecode::address_t main_ip;
        std::string error = "";

        void exit();

        // GC
        void add_object(Values::Object *obj);
        void mark_object(Values::Value value);
        void mark_values();
        void delete_values();
        void run_gc();
        // Queue a GC run when the stack is emptied
        bool gc_queue = false;
        int gc_size = 0;
    public:
        Runtime(Bytecode::Chunk &main);

        void init_global_pool(size_t num_globals);

        // Generate new constant in the pool and return index in the constant pool
        Bytecode::variable_index_t new_constant(Values::Value value);
        // Add a function to the function list
        void add_function(RuntimeFunction &chunk);

        inline Bytecode::Chunk * get_main() { return &this->main; };
        inline Values::Value     get_constant(Bytecode::constant_index_t index) const { return this->constants.at(index); };

        void log_instructions();
        int run();

        ~Runtime();
};

#endif