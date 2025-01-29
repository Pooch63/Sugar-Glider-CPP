#ifndef _SG_CPP_RUNTIME_HPP
#define _SG_CPP_RUNTIME_HPP

#include "natives.hpp"
#include "../ir/bytecode.hpp"
#include "../value.hpp"

#include <vector>

// Linked list of runtime values that we can clean up later
struct RuntimeValue {
    Values::Value value;
    // Used by the GC
    bool marked_to_save;
    RuntimeValue *next;
};
class Runtime {
    private:
        Bytecode::Chunk main = Bytecode::Chunk();

        RuntimeValue *runtime_values;
        std::vector<Values::Value> constants = std::vector<Values::Value>();

        /* Make a separate copy for natives so that each individual runtime can update native namespaces */
        std::array<Values::Value, Natives::native_count> natives = std::array<Values::Value, Natives::native_count>();

        std::vector<Values::Value> stack = std::vector<Values::Value>();

        Values::Value stack_pop();
        void exit();
    public:
        Runtime(Bytecode::Chunk &main);

        // Generate new constant in the pool and return index in the constant pool
        Bytecode::variable_index_t new_constant(Values::Value value);

        inline Bytecode::Chunk *get_main() { return &this->main; };
        inline Values::Value    get_constant(Bytecode::constant_index_t index) const { return this->constants.at(index); };

        int run();
};

#endif