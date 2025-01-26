#include "bytecode.hpp"
#include "../utils.hpp"

#ifdef DEBUG
#include <iostream>
#include <string>
#endif

using namespace Bytecode;

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

const char* instruction_to_string(OpCode code) {
    switch (code) {
        case OpCode::OP_POP: return "POP";
        case OpCode::OP_GOTO: return "GOTO";
        case OpCode::OP_POP_JIZ: return "POP_JIZ";
        case OpCode::OP_POP_JNZ: return "POP_JNZ";
        case OpCode::OP_BIN: return "BIN_OP";
        case OpCode::OP_UNARY: return "UNARY_OP";
        case OpCode::OP_TRUE: return "OP_TRUE";
        case OpCode::OP_FALSE: return "OP_FALSE";
        case OpCode::OP_NULL: return "OP_NULL";
        case OpCode::OP_NUMBER: return "OP_NUMBER";
        case OpCode::OP_LOAD_CONST: return "LOAD_CONST";
        case OpCode::OP_RETURN: return "RETURN";
        case OpCode::OP_CALL: return "CALL";
        case OpCode::OP_EXIT: return "EXIT";

        default:
            throw sg_assert_error("Unknown bytecode instruction to log to string");
    }
}

#include "../../lib/rang.hpp"

/* How long the area for the instruction name should be when we're logging */
static const int INSTRUCTION_NAME_LENGTH = 30;
static const int MAX_ARGUMENT_LENGTH = IR_LABEL_LENGTH + 5;
static const rang::fg comment_c = rang::fg::green;
int Chunk::print_instruction(uint current_byte_index) {
    OpCode code = static_cast<OpCode>(this->read_byte(current_byte_index));
    /* The number of bytes we'll need to read. The first is the instruction */
    int read_count = 1;

    std::string output = "";

    // Add the instruction name
    output = instruction_to_string(code);

    // Add spaces
    uint instr_name_length = output.size();
    for (uint space = 0; space < INSTRUCTION_NAME_LENGTH - instr_name_length; space += 1) {
        output += " ";
    }

    std::string comment = "";
    // Add the comment if necessary
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
            constant_index_t constant_index = this->read_value<constant_index_t>(current_byte_index + read_count);
            output += std::to_string(constant_index);
            read_count += sizeof(uint32_t);
        }
            break;
        case OpCode::OP_CALL:
        {
            call_arguments_t arg_count = this->read_value<call_arguments_t>(current_byte_index + read_count);
            output += std::to_string(arg_count);
            read_count += sizeof(address_t);
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

    if (comment.size() > 0) {
        for (int space = 0; space < MAX_ARGUMENT_LENGTH - static_cast<int>(get_string_length_as_utf32(comment)); space += 1) {
            std::cout << ' ';
        }
        std::cout << comment_c << "; " << comment;
    }

    std::cout << rang::style::reset;

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
        std::cout << "  " << std::to_string(byte_index) << " |  ";

        /* Log the instruction, and add the number of bytes we advanced to the byte index. */
        byte_index += this->print_instruction(byte_index);

        std::cout << '\n';
    }
    
    std::cout << "-------------------------------------------------------" << std::endl;
};
#endif