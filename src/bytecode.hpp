#ifndef _SGCPP_BYTECODE_HPP
#define _SGCPP_BYTECODE_HPP

#include "bit-converter.hpp"
#include "globals.hpp"
#include "operations.hpp"
#include "value.hpp"

#include <vector>

namespace Instruction {
    enum OpCode {
        /* Pops the top value on the stack.
        0 arguments */
        OP_POP,

        /* Goes to the byte at the specified index.
            This byte with be interpreter as the next instruction to perform.
            Argument is 4 bytes long, uint32_t. */
        OP_GOTO,
        /* If the top value on the stack is falsey, goes to the byte at the specified index.
            This byte will be interpreted as the next instruction to perform.
            Then, it pops the top value on the stack. Argument is 4 bytes long, uint32_t. */
        OP_POP_JIZ,
        /* If the top value on the stack is truthy, goes to the byte at the specified index.
            This byte with be interpreter as the next instruction to perform.
            Then, it pops the top value on the stack. Argument is 4 bytes long, uint32_t. */
        OP_POP_JNZ,

        /* Reads the two topmost values on the stack. Performs the specified binary operation,
            where the bottom-most of the two values is the first operand. Then, pops the two
            topmost values on the stack, and pushes the result of the operation onto the stack.
            For example, if the stack was a, b, c, this instruction would pop b and c and perform
            the operation b (?) c. The stack would become a, (b (?) c).
            Argument is 1 byte long, BINARY_OP_TYPE. */
        OP_BIN,
        /* Reads the topmost value on the stack. Performs the specified unary operation,
            then pops the topmost value on the stack. Finally, pushes the result of the operation
            onto the stack.
            For example, if the stack was a, b, this instruction would pop b and perform the
            operation (?)b. The stack would become a, (?)b.
            Argument is 1 byte long, UNARY_OP_TYPE. */
        OP_UNARY,

        /* Push a “true” value onto the stack
            0 arguments */
        OP_TRUE,
        /* Push a “false” value onto the stack
            0 arguments */
        OP_FALSE,
        /* Push a “null” value onto the stack.
            0 arguments */
        OP_NULL,

        /* Push a specified number value onto the stack.
            Argument is sizeof(Value::number_t) bytes long (so 8 if it's a double), the number value to pass. */
        OP_NUMBER,

        /* Push the constant at this index in the constant array onto the stack.
            Constant must be a value of unknown size at runtime, so not a boolean, null, nor number.
    OpCode        Argument is 4 bytes, uint32_t. */
        OP_LOAD_CONST,
    };

    typedef std::vector<uint8_t> bytecode_t;
    class Chunk {
        private:
            bytecode_t code = bytecode_t();

            #ifdef DEBUG
            /* From the current byte index in the code, log the instruction
                and any arguments it has. Then, return the number of bytes we read,
                starting from the current byte, in order to log it. */
            int print_instruction(int current_byte_index);
            #endif
        public:
            /* Push an enum into the code.
                Will push only the first byte of the enum value. */
            template<typename enum_t>
            void push_small_enum(enum_t value) {
                this->code.push_back(get_bottom_byte<enum_t>(value));
            }
            /* Push the first byte of an opcode. */
            void push_opcode(OpCode code);
            void push_bin_op_type(Operations::BinOpType type);
            void push_unary_op_type(Operations::UnaryOpType type);

            /* Push a uint32 into the code in individual bytes. */
            void push_uint32(uint32_t data);
            /* Push a number of type Value::number_t into the code in individual bytes */
            void push_number_value(Value::number_t data);

            /* Read a single unsigned byte at the specified index */
            uint8_t read_byte(int index) const;
            /* Read a uint32, starting from the specified index */
            uint32_t read_uint32(int current_byte_index) const;
            /* Read a value of type number_t, starting from the specified index */
            Value::number_t read_number_value(int current_byte_index) const;

            #ifdef DEBUG
            /* Log representation of bytecode to console */
            void print_code();
            #endif

    };
};

#endif
