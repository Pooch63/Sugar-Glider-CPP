#include "bytecode.hpp"

#ifdef DEBUG
#include <iostream>
#include <string>
#endif

using namespace Instruction;

void Chunk::push_opcode(OpCode code) {
    this->push_small_enum<OpCode>(code);
}
/* Note that push_bin_op_type and puush_unary_op_type only push the first byte
    of the code because, as of now, there are AT MOST 256 binary op types and
    AT MOST 256 unary op types. */
void Chunk::push_bin_op_type(Operations::BinOpType type) {
    this->push_small_enum<Operations::BinOpType>(type);
};
void Chunk::push_unary_op_type(Operations::UnaryOpType type) {
    this->push_small_enum<Operations::UnaryOpType>(type);
};

uint8_t Chunk::read_byte(uint current_byte_index) const {
    return this->code.at(current_byte_index);
}

#ifdef DEBUG
/* Get the number of digits in a decimal number */
static uint get_digits(uint number) {
    return number < 10 ? 1 :
            number < 100 ? 2 :
            number < 1'000 ? 3 :
            number < 10'000 ? 4 :
            number < 100'000 ? 5 :
            number < 1'000'000 ? 6 :
            number < 10'000'000 ? 7 :
            number < 100'000'000 ? 8 : 9;
}

#include "operations.hpp"

/* How long the area for the instruction name should be when we're logging */
static int INSTRUCTION_NAME_LENGTH = 30;
int Chunk::print_instruction(uint current_byte_index) {
    OpCode code = static_cast<OpCode>(this->read_byte(current_byte_index));
    /* The number of bytes we'll need to read. The first is the instruction */
    int read_count = 1;

    std::string output = "";

    // Add the instruction name
    switch (code) {
        case OpCode::OP_POP:
            output = "POP";
            break;
        case OpCode::OP_GOTO:
            output = "GOTO";
            break;
        case OpCode::OP_POP_JIZ:
            output = "POP_JIZ";
            break;
        case OpCode::OP_POP_JNZ:
            output = "POP_JNZ";
            break;
        case OpCode::OP_BIN:
            output = "BIN_OP";
            break;
        case OpCode::OP_UNARY:
            output = "UNARY_OP";
            break;
        case OpCode::OP_TRUE:
            output = "OP_TRUE";
            break;
        case OpCode::OP_FALSE:
            output = "OP_FALSE";
            break;
        case OpCode::OP_NULL:
            output = "OP_NULL";
            break;
        case OpCode::OP_NUMBER:
            output = "OP_NUMBER";
            break;
        case OpCode::OP_LOAD_CONST:
            output = "LOAD_CONST";
            break;
        case OpCode::OP_EXIT:
            output = "EXIT";
            break;
    }

    // Add spaces
    uint instr_name_length = output.size();
    for (uint space = 0; space < INSTRUCTION_NAME_LENGTH - instr_name_length; space += 1) {
        output += " ";
    }

    // Add the argument information, if it needs it
    switch (code) {
        case OpCode::OP_GOTO:
        case OpCode::OP_POP_JIZ:
        case OpCode::OP_POP_JNZ:
            output += std::to_string(this->read_address(current_byte_index + read_count));
            read_count += sizeof(address_t);
            break;
        case OpCode::OP_BIN:
        {
            Operations::BinOpType type = static_cast<Operations::BinOpType>(this->read_byte(current_byte_index + read_count));
            output += std::to_string(static_cast<uint>(type));
            output += " (";
            output += Operations::bin_op_to_string(type);
            output += ')';
            read_count += 1;
        }
            break;
        case OpCode::OP_UNARY:
        {
            Operations::UnaryOpType type = static_cast<Operations::UnaryOpType>(this->read_byte(current_byte_index + read_count));
            output += std::to_string(static_cast<uint>(type));
            output += " (";
            output += Operations::unary_op_to_string(type);
            output += ')';
            read_count += 1;
        }
            break;
        case OpCode::OP_LOAD_CONST:
        {
            uint32_t constant_index = this->read_uint32(current_byte_index + read_count);
            output += std::to_string(constant_index);
            read_count += sizeof(uint32_t);
        }
            break;
        case OpCode::OP_NUMBER:
        {
            Values::number_t number = this->read_number_value(current_byte_index + read_count);
            output += std::to_string(number);
            read_count += sizeof(Values::number_t);
        }
            break;

        default: break;
    }

    std::cout << output;

    return read_count;
}

void Chunk::print_code() {
    uint code_count_length = get_digits(this->code.size());

    std::cout << "-------------------------------------------------------\n";
    // Don't increment the loop, because print instruction always increases it by at least one
    for (uint byte_index = 0; byte_index < this->code.size();) {
        /* Log instruction number */
        uint index_length = get_digits(byte_index);
        for (uint space = 0; space < code_count_length - index_length; space += 1) {
            std::cout << ' ';
        }
        std::cout << std::to_string(byte_index) << " |  ";

        /* Log the instruction, and add the number of bytes we advanced to the byte index. */
        byte_index += this->print_instruction(byte_index);

        std::cout << '\n';
    }
    
    std::cout << "-------------------------------------------------------" << std::endl;
};
#endif