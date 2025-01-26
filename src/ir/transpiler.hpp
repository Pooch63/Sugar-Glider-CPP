#ifndef _SGCPP_TRANSPILER_HPP
#define _SGCPP_TRANSPILER_HPP

#include "bytecode.hpp"
#include "intermediate.hpp"
#include "../runtime/runtime.hpp"

/* Transpile label intermediate to bytecode.
    Use a class to store state necessary for the transformation.
    No need to save the class. Transpiler().transpile_label_to_bytecode() */
class Transpiler {
    private:
        Runtime &runtime;
        Bytecode::Chunk *chunk;
        /* A map of labels to their byte index */
        std::unordered_map<Intermediate::label_index_t*, Bytecode::address_t> label_starts = std::unordered_map<Intermediate::label_index_t*, Bytecode::address_t>();

        void transpile_ir_instruction(Intermediate::Instruction instr);
    public:
        Transpiler(Runtime &runtime);

        void transpile_label_to_bytecode(Intermediate::Block& labels);
};

#endif