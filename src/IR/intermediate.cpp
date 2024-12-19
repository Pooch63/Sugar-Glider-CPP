#include "intermediate.hpp"
#include "../utils.hpp"

using namespace Intermediate;

#ifdef DEBUG
#include <cassert>
#include <iostream>

Instruction::Instruction(InstrCode code) : code(code) {};
Instruction::Instruction(InstrCode code, address_t label) : code(code), payload(ir_instruction_arg_t{ .label = label }) {};
Instruction::Instruction(InstrCode code, Operations::BinOpType type) :
    code(code), payload(ir_instruction_arg_t{ .bin_op = type }) {};
Instruction::Instruction(InstrCode code, Operations::UnaryOpType type) :
    code(code), payload(ir_instruction_arg_t{ .unary_op = type }) {};
Instruction::Instruction(Values::number_t number) :
    code(Intermediate::INSTR_NUMBER), payload(ir_instruction_arg_t{ .number = number }) {};



const char* Intermediate::instr_type_to_string(InstrCode code) {
    switch (code) {
        case InstrCode::INSTR_POP:
            return "POP";
        case InstrCode::INSTR_GOTO:
            return "GOTO";
        case InstrCode::INSTR_POP_JIZ:
            return "OP_POP_JIZ";
        case InstrCode::INSTR_POP_JNZ:
            return "OP_POP_JNZ";
        case InstrCode::INSTR_BIN_OP:
            return "BIN_OP";
        case InstrCode::INSTR_UNARY_OP:
            return "UNARY_OP";
        case InstrCode::INSTR_TRUE:
            return "INSTR_TRUE";
        case InstrCode::INSTR_FALSE:
            return "INSTR_FALSE";
        case InstrCode::INSTR_NULL:
            return "INSTR_NULL";
        case InstrCode::INSTR_NUMBER:
            return "INSTR_NUMBER";
        case InstrCode::INSTR_EXIT:
            return "EXIT";
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};

/* Length of instruction name in the console */
static uint INSTRUCTION_NAME_LENGTH = 30;

static void log_instruction(Instruction instr) {
    std::string type = Intermediate::instr_type_to_string(instr.code);
    
    std::cout << type;
    for (uint space = type.size(); space < INSTRUCTION_NAME_LENGTH; space += 1) {
        std::cout << ' ';
    }

    /* The argument */
    switch (instr.code) {
        case InstrCode::INSTR_GOTO:
        case InstrCode::INSTR_POP_JIZ:
        case InstrCode::INSTR_POP_JNZ:
            std::cout << ".L" << instr.payload.label;
            break;
        case InstrCode::INSTR_BIN_OP:
        {
            Operations::BinOpType type = instr.payload.bin_op;
            std::cout << std::to_string(static_cast<uint>(type));
            std::cout << " (";
            std::cout << Operations::bin_op_to_string(type);
            std::cout << ')';
        }
            break;
        case InstrCode::INSTR_UNARY_OP:
        {
            Operations::UnaryOpType type = instr.payload.unary_op;
            std::cout << std::to_string(static_cast<uint>(type));
            std::cout << " (";
            std::cout << Operations::unary_op_to_string(type);
            std::cout << ')';
        }
            break;
        case InstrCode::INSTR_NUMBER:
        {
            std::cout << instr.payload.number;
        }
            break;
        default: break;
    }
    std::cout << '\n';
}

Block::Block() {
    this->labels.push_back(intermediate_set_t());
}
void Block::log_block() const {
    size_t size = 0;
    for (intermediate_set_t set : this->labels) size += set.size();

    uint code_count_length = get_digits(size);

    uint instr_number = 0;

    std::cout << "-------------------------------------------------------\n";
    std::cout << "                          IR                           \n";
    // Don't increment the loop, because print instruction always increases it by at least one
    for (uint set_count = 0; set_count < this->labels.size(); set_count += 1) {
        std::cout << '\n';
        std::cout << ".L" << set_count << ":\n";

        for (Instruction instr : this->labels.at(set_count)) {
            /* Log instruction number */
            uint index_length = get_digits(instr_number);
            for (uint space = 0; space < code_count_length - index_length; space += 1) {
                std::cout << ' ';
            }
            std::cout << "  " << std::to_string(instr_number) << " |  ";

            log_instruction(instr);
            instr_number += 1;
        }
    }
    
    std::cout << "-------------------------------------------------------" << std::endl;
}

#endif

void Block::new_label() {
    this->labels.push_back(intermediate_set_t());
}
void Block::add_instruction(Intermediate::Instruction instruction) {
    this->labels.back().push_back(instruction);
}