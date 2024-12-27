/* Intermediate representation */

#ifndef _SGCPP_CHUNK_HPP
#define _SGCPP_CHUNK_HPP

#include "bytecode.hpp"
#include "../compiler/scopes.hpp"
#include "../globals.hpp"
#include "../utils.hpp"

#include <vector>

/* The number of characters that can be used in a random label name.
    Generating two identical 20-character labels from these 64 codepoints 
   is astronomically unlikely -- less than winning the Mega Millions 5 times 
   in a row and being stricken by lightning 19 times after. */
#define CHAR_LABEL_COUNT 64

using Bytecode::OpCode;

namespace Intermediate {
    /* Many of these op codes are exactly the same as the ones in the
        final bytecode. If an instruction is not explained here,
        go to bytecode.hpp for an explanation. */
    enum InstrCode {
        INSTR_POP,

        /* These intermediate representation commands have an address they point to */
        INSTR_POP_JNZ,
        INSTR_POP_JIZ,
        INSTR_GOTO,

        /* Has a binary op type argument */
        INSTR_BIN_OP,
        /* Has a unary up type argument */
        INSTR_UNARY_OP,

        INSTR_TRUE,
        INSTR_FALSE,
        INSTR_NULL,
        INSTR_NUMBER,

        INSTR_LOAD,
        INSTR_STORE,

        INSTR_EXIT
    };

    #ifdef DEBUG
    const char* instr_type_to_string(InstrCode code);
    #endif

    typedef std::string label_index_t;

    union ir_instruction_arg_t {
        /* For jump commands */
        label_index_t* label;
        Operations::BinOpType bin_op;
        Operations::UnaryOpType unary_op;
        Values::number_t number;

        Scopes::Variable variable;
    };

    struct Instruction {
        Intermediate::InstrCode code;
        ir_instruction_arg_t payload;

        Instruction(Intermediate::InstrCode code);
        Instruction(Intermediate::InstrCode code, label_index_t* label);
        Instruction(Intermediate::InstrCode code, Operations::BinOpType bin_op);
        Instruction(Intermediate::InstrCode code, Operations::UnaryOpType unary_op);
        Instruction(InstrCode code, Scopes::Variable variable);
        /* There is only one instruction that takes this number. */
        Instruction(Values::number_t number);

        bool is_truthy_constant() const;
        bool is_constant() const;
        inline bool is_jump() const {
            return this->code == InstrCode::INSTR_GOTO ||
                this->code == InstrCode::INSTR_POP_JIZ ||
                this->code == InstrCode::INSTR_POP_JNZ; };
        
        inline bool has_number_payload() const { return this->code == InstrCode::INSTR_NUMBER; };
        // inline bool has_string_payload() const { return this->code == InstrCode::INSTR_STRING; };
        

        Values::number_t        get_number() const;
        label_index_t*          get_address() const;
        Operations::BinOpType   get_bin_op() const;
        Operations::UnaryOpType get_unary_op() const;

        /* Convert payload to a value object. Only call if it CAN be converted, e.g.
            if it's a number, true, false, null, etc. */
        Values::Value payload_to_value() const;

        /* Convert a value to an instruction that loads it onto the stack. */
        static Instruction value_to_instruction(Values::Value value);
    };

    typedef std::vector<Intermediate::Instruction> intermediate_set_t;

    struct Label {
        label_index_t* name;
        intermediate_set_t instructions = intermediate_set_t();

        Label(label_index_t* name);
    };

    class Block {
        private:
            /* Used to generate label names */
            static std::uniform_int_distribution<uint32_t> label_generator;

            /* Labels. Once we leave a label, we can no longer compile into it.
                We can also only compile into one label at once.
                Thus, we don't need a hashmap. Even though the keys are strings,
                we don't*/
            std::vector<Label> labels = std::vector<Label>();
        public:
            Block();

            /* Generate a new label with a randomly-generated index. */
            label_index_t* new_label();
            /* Generate a new label with a given index. */
            void new_label(label_index_t* index);
            /* Get the label in the vector at the specified numerical index.
                NOT the label index. */
            Label &get_label_at_numerical_index(uint index);
            inline size_t label_count() const { return this->labels.size(); };

            /* Add an instruction to the last label. If there are no labels, one will be created. */
            void add_instruction(Intermediate::Instruction instruction);

            void log_block() const;

            inline auto begin() const { return this->labels.begin(); };
            inline auto end() const { return this->labels.end(); };

            /* Generate a valid label name. It DOES NOT guarantee that this label
                name is not already a label name, but it basically is.
                Dynamically allocates the index, so if you do not add a label with this name,
                YOU are responsible for freeing the memory. */
            label_index_t* gen_label_name() const;

            ~Block();
    };
};

#endif