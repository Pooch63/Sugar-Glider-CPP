/* Intermediate representation */

#ifndef _SGCPP_CHUNK_HPP
#define _SGCPP_CHUNK_HPP

#include "bytecode.hpp"
#include "../globals.hpp"
#include "../utils.hpp"

#ifdef DEBUG
#include <cassert> // For getters
#endif

#include <unordered_map>
#include <vector>

/* The number of characters that can be used in a random label name.
    Generating two identical 20-character labels from these 64 codepoints 
   is astronomically unlikely -- less than winning the Mega Millions 5 times 
   in a row and being stricken by lightning 19 times after. On the nonexistent
   chance it does happen, compilation WILL break, but you just have to compile again. */
#define CHAR_LABEL_COUNT 64

using Bytecode::OpCode;

namespace Intermediate {
    enum VariableType {
        CONSTANT,
        MUTABLE
    };
    struct Variable {
        std::string* name;
        VariableType type;
        int scope;

        bool operator==(const Variable &var) const {
            return *name == *var.name && type == var.type && scope == var.scope;
        };
    };
    struct VariableHasher {
        // Don't include the type in the hashing function because that is bound to the name
        // and scope. There can't be two variables whose only difference is their type.
        size_t operator()(const Variable &var) const {
            return std::hash<std::string>()(*var.name) ^ std::hash<int>()(var.scope); 
        }
    };

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
        INSTR_STRING,

        INSTR_LOAD,
        INSTR_STORE,

        /* Argument is number of arguments that are used to call the function.
            Top value of stack must be the function to call.
            The next n values must be the n arguments, e.g.
            x(1, 2) = a stack of:
            x
            2
            1 */
        INSTR_CALL,

        INSTR_EXIT
    };

    #ifdef DEBUG
    const char* instr_type_to_string(InstrCode code);
    #endif

    typedef std::string label_index_t;

    union ir_instruction_arg_t {
        /* For jump commands */
        label_index_t* label;
        std::string* str;
        Operations::BinOpType bin_op;
        Operations::UnaryOpType unary_op;
        Values::number_t number;

        uint num_arguments;

        Variable variable;
    };

    struct Instruction {
        InstrCode code;
        ir_instruction_arg_t payload;

        Instruction(InstrCode code);
        /* Used for strings, and also for jump commands, since a string
            is the label index type.
            If the label type is ever changed, then another overload will be needed. */
        Instruction(InstrCode code, std::string* payload);
        Instruction(InstrCode code, Operations::BinOpType bin_op);
        Instruction(InstrCode code, Operations::UnaryOpType unary_op);
        Instruction(InstrCode code, Variable variable);
        Instruction(InstrCode code, uint argument);
        /* There is only one instruction that takes this number. */
        Instruction(Values::number_t number);

        bool is_truthy_constant() const;
        bool is_constant() const;
        /* Is this instruction a static flow instruction that loads a value onto the stack? */
        bool is_static_flow_load() const;
        inline bool is_jump() const {
            return this->code == InstrCode::INSTR_GOTO ||
                this->code == InstrCode::INSTR_POP_JIZ ||
                this->code == InstrCode::INSTR_POP_JNZ; };
        
        inline bool has_number_payload() const { return this->code == InstrCode::INSTR_NUMBER; };
        // inline bool has_string_payload() const { return this->code == InstrCode::INSTR_STRING; };
        
        // Getters >
        inline Values::number_t get_number() const {
            #ifdef DEBUG
            assert(this->code == InstrCode::INSTR_NUMBER);
            #endif
            return this->payload.number;
        }
        inline label_index_t* get_address() const {
            #ifdef DEBUG
            assert(this->is_jump());
            #endif
            return this->payload.label;
        }
        inline Operations::BinOpType get_bin_op() const {
            #ifdef DEBUG
            assert(this->code == InstrCode::INSTR_BIN_OP);
            #endif
            return this->payload.bin_op;
        }
        inline Operations::UnaryOpType get_unary_op() const {
            #ifdef DEBUG
            assert(this->code == InstrCode::INSTR_UNARY_OP);
            #endif
            return this->payload.unary_op;
        }
        inline uint get_argument_count() const {
            #ifdef DEBUG
            assert(this->code == InstrCode::INSTR_CALL);
            #endif
            return this->payload.num_arguments;
        }

        // < Getters

        /* Convert payload to a value object. Only call if it CAN be converted, e.g.
            if it's a number, true, false, null, etc. */
        Values::Value payload_to_value() const;

        /* If it's a string, free the payload */
        void free_payload();

        /* Convert a value to an instruction that loads it onto the stack. */
        static Instruction value_to_instruction(Values::Value value);
    };
    void log_instruction(Instruction instr);

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
                we don't enter a label after closing it.
                After each label, a GOTO statement must bring it to another label.
                E.g.,
                    .L0:
                        ...
                        GOTO .L1
                    .L1:
                        ...
                Adding a label will automatically add a goto to advance it if this is not
                done automatically. */
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