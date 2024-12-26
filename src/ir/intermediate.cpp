#include "intermediate.hpp"
#include "../utils.hpp"

using namespace Intermediate;

Instruction::Instruction(InstrCode code) : code(code) {};
Instruction::Instruction(InstrCode code, address_t label) : code(code), payload(ir_instruction_arg_t{ .label = label }) {};
Instruction::Instruction(InstrCode code, Operations::BinOpType type) :
    code(code), payload(ir_instruction_arg_t{ .bin_op = type }) {};
Instruction::Instruction(InstrCode code, Operations::UnaryOpType type) :
    code(code), payload(ir_instruction_arg_t{ .unary_op = type }) {};
Instruction::Instruction(InstrCode code, Variable variable) :
    code(code), payload(ir_instruction_arg_t{ .variable = variable }) {};

Instruction::Instruction(Values::number_t number) :
    code(Intermediate::INSTR_NUMBER), payload(ir_instruction_arg_t{ .number = number }) {};

Values::number_t Instruction::get_number() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_NUMBER);
    #endif
    return this->payload.number;
}
address_t Instruction::get_address() const {
    #ifdef DEBUG
    assert(this->is_jump());
    #endif
    return this->payload.label;
}
Operations::BinOpType Instruction::get_bin_op() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_BIN_OP);
    #endif
    return this->payload.bin_op;
}
Operations::UnaryOpType Instruction::get_unary_op() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_UNARY_OP);
    #endif
    return this->payload.unary_op;
}


bool Instruction::is_truthy_constant() const {
    return this->code == InstrCode::INSTR_TRUE || this->code == InstrCode::INSTR_FALSE ||
        (this->has_number_payload() && this->get_number() != 0);
}
bool Instruction::is_constant() const {
    return this->code == InstrCode::INSTR_TRUE ||
        this->code == InstrCode::INSTR_FALSE ||
        this->code == InstrCode::INSTR_NULL ||
        this->code == InstrCode::INSTR_NUMBER;
}

Values::Value Instruction::payload_to_value() const {
    switch (this->code) {
        case InstrCode::INSTR_NUMBER:
            return Values::Value(Values::ValueType::NUMBER, this->get_number());
        case InstrCode::INSTR_TRUE:
            return Values::Value(Values::ValueType::TRUE);
        case InstrCode::INSTR_FALSE:
            return Values::Value(Values::ValueType::FALSE);
        default:
            #ifdef DEBUG
            /* Can't convert this instruction to a value */
            assert(false);
            #endif
    }
};

Instruction Instruction::value_to_instruction(Values::Value value) {
    switch (value.get_type()) {
        case Values::ValueType::NUMBER:
            return Instruction(value.get_number());
        case Values::ValueType::TRUE:
            return Instruction(InstrCode::INSTR_TRUE);
        case Values::ValueType::FALSE:
            return Instruction(InstrCode::INSTR_FALSE);
        default:
            #ifdef DEBUG
            /* Can't convert this to an instruction */
            assert(false);
            #endif
    }
};

#ifdef DEBUG
#include <cassert>
#include <iostream>

const char* Intermediate::instr_type_to_string(InstrCode code) {
    switch (code) {
        case InstrCode::INSTR_POP:
            return "POP";
        case InstrCode::INSTR_GOTO:
            return "GOTO";
        case InstrCode::INSTR_POP_JIZ:
            return "POP_JIZ";
        case InstrCode::INSTR_POP_JNZ:
            return "POP_JNZ";
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
        case InstrCode::INSTR_LOAD:
            return "LOAD";
        case InstrCode::INSTR_STORE:
            return "STORE";
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
        case InstrCode::INSTR_LOAD:
        case InstrCode::INSTR_STORE:
        {
            std::cout << *instr.payload.variable.name;
        }
            break;
        default: break;
    }
    std::cout << '\n';
}

Block::Block() {
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

intermediate_set_t &Block::get_label(uint index) { return this->labels.at(index); }

void Block::new_label() {
    this->labels.push_back(intermediate_set_t());
}
void Block::add_instruction(Intermediate::Instruction instruction) {
    if (this->labels.size() == 0) this->new_label();
    this->labels.back().push_back(instruction);
}