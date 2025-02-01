#ifndef _SGCPP_TRANSPILER_HPP
#define _SGCPP_TRANSPILER_HPP

#include "bytecode.hpp"
#include "intermediate.hpp"
#include "../runtime/runtime.hpp"

struct Jump_Argument {
    Bytecode::address_t byte_address;
    Intermediate::label_index_t *label;

    Jump_Argument(Bytecode::address_t, Intermediate::label_index_t *);
};

/* Transpile label intermediate to bytecode.
    Use a class to store state necessary for the transformation.
    No need to save the class. Transpiler().transpile_label_to_bytecode() */
class Transpiler {
    private:
        Runtime &runtime;
        Bytecode::Chunk *chunk;
        /* A map of labels to their byte index */
        std::unordered_map<Intermediate::label_index_t, Bytecode::address_t> label_starts = std::unordered_map<Intermediate::label_index_t, Bytecode::address_t>();
        /* A map of byte indices and the bytecode as well as the name of the label whose index they jump to */
        std::vector<Jump_Argument> jump_arguments = std::vector<Jump_Argument>();

        /* A map of IR variables to var indices. Remember to use custom hasher for Variable class. */
        std::unordered_map<Intermediate::Variable, Bytecode::variable_index_t, Intermediate::VariableHasher> variables =
            std::unordered_map<Intermediate::Variable, Bytecode::variable_index_t, Intermediate::VariableHasher>();

        void transpile_variable_instruction(Intermediate::Instruction instr);
        void transpile_ir_instruction(Intermediate::Instruction instr);
        void transpile_single_block(Intermediate::Block *labels);
    public:
        Transpiler(Runtime &runtime);

        void transpile_ir_to_bytecode(Intermediate::LabelIR &main);

        inline size_t num_variable_slots() const { return this->variables.size(); };
};

#endif