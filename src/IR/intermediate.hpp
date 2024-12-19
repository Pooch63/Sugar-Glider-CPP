/* Intermediate representation */

#ifndef _SGCPP_CHUNK_HPP
#define _SGCPP_CHUNK_HPP

#include "bytecode.hpp"
#include "../globals.hpp"

#include <vector>

using Bytecode::address_t, Bytecode::OpCode;

namespace Intermediate {
    /* Many of these op codes are exactly the same as the ones in the
        final bytecode. If an instruction is not explained here,
        go to bytecode.hpp for an explanation. */
    enum InstrCode {
        INSTR_POP,

        /* These IR commands have an address they point to */
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

        INSTR_EXIT
    };

    #ifdef DEBUG
    const char* instr_type_to_string(InstrCode code);
    #endif

    union ir_instruction_arg_t {
        /* For jump commands */
        address_t label;
        Operations::BinOpType bin_op;
        Operations::UnaryOpType unary_op;
        Values::number_t number;
    };

    struct Instruction {
        Intermediate::InstrCode code;
        ir_instruction_arg_t payload;

        Instruction(Intermediate::InstrCode code);
        Instruction(Intermediate::InstrCode code, address_t label);
        Instruction(Intermediate::InstrCode code, Operations::BinOpType bin_op);
        Instruction(Intermediate::InstrCode code, Operations::UnaryOpType unary_op);
        /* There is only one instruction that takes this number. */
        Instruction(Values::number_t number);
    };

    typedef std::vector<Intermediate::Instruction> intermediate_set_t;
    class Block {
        private:
            /* Labels. Once we leave a label, we can no longer compile into it.
                We can also only compile into one label at once. */
            std::vector<intermediate_set_t> labels = std::vector<intermediate_set_t>();
        public:
            Block();

            void new_label();
            inline size_t label_count() const { return this->labels.size(); };

            void add_instruction(Intermediate::Instruction instruction);

            void log_block() const;
    };
};

#endif