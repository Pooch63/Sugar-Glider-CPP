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

void Chunk::push_uint32(uint32_t data) {
    union {
        uint32_t payload;
        /* Don't take for granted the uint32_t. Certain architectures (i.e., Arduino), may not
            have 4-byte integers. */
        uint8_t bytes[sizeof(uint32_t)];
    } reader;
    reader.payload = data;

    for (uint_fast8_t byte = 0; byte < sizeof(uint32_t); byte += 1) {
        this->code.push_back(reader.bytes[byte]);
    }
};

void Chunk::push_number_value(Value::number_t data) {
    union {
        Value::number_t payload;
        uint8_t bytes[sizeof(Value::number_t)];
    } reader;
    reader.payload = data;

    for (uint_fast8_t byte = 0; byte < sizeof(Value::number_t); byte += 1) {
        this->code.push_back(reader.bytes[byte]);
    }
};

uint8_t Chunk::read_byte(int current_byte_index) const {
    return this->code.at(current_byte_index);
}
uint32_t Chunk::read_uint32(int current_byte_index) const {
    #ifdef _SGCPP_SYSTEM_IS_BIG_ENDIAN
    #else
    union {
        uint32_t data;
        uint8_t code[sizeof(uint32_t)];
    } payload;

    for (uint_fast8_t byte_index = 0; byte_index < sizeof(uint32_t); byte_index += 1) {
        payload.code[byte_index] = this->read_byte(current_byte_index + byte_index);
    }

    return payload.data;
    #endif
}
Value::number_t Chunk::read_number_value(int current_byte_index) const {
    #ifdef _SGCPP_SYSTEM_IS_BIG_ENDIAN
    #else
    union {
        Value::number_t data;
        uint8_t code[sizeof(Value::number_t)];
    } payload;

    for (uint_fast8_t byte_index = 0; byte_index < sizeof(Value::number_t); byte_index += 1) {
        payload.code[byte_index] = this->read_byte(current_byte_index + byte_index);
    }

    return payload.data;
    #endif
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
int Chunk::print_instruction(int current_byte_index) {
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
            output += std::to_string(this->read_byte(current_byte_index + read_count));
            read_count += 1;
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
            Value::number_t number = this->read_number_value(current_byte_index + read_count);
            output += std::to_string(number);
            read_count += sizeof(Value::number_t);
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