#ifndef _SGCPP_BYTECODE_HPP
#define _SGCPP_BYTECODE_HPP

#include "../bit-converter.hpp"
#include "../globals.hpp"
#include "../operations.hpp"
#include "../value.hpp"

#include <vector>

/* Use assert for Chunk template helpers */
#ifdef DEBUG
#include <cassert>
#endif

// Forward declaration for class descibed in runtime/runtime.hpp
// This is because the bytecode logger function depends on the runtime class
class Runtime;

namespace Bytecode {
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
            Argument is sizeof(Values::number_t) bytes long (so 8 if it's a double), the number value to pass. */
        OP_NUMBER,

        /* Push the constant at this index in the constant array onto the stack.
            Constant must be a value of unknown size at runtime, so not a boolean, null, nor number.
            Argument is constant_index_t. */
        OP_LOAD_CONST,

        /* Return from the current function */
        OP_RETURN,
        /* Argument is call_arguments_t, number of arguments that are used to call the function.
            Top value of stack must be the function to call.
            The next n values must be the n arguments, e.g.
            x(1, 2) = a stack of:
            x
            2
            1 */
        OP_CALL,

        /* Push the value of the variable at the specified index on top of the stack.
            Argument is sizeof(variable_index_t) bytes long, the variable index */
        OP_LOAD,
        /* Store the topmost value on the stack into the specified variable index.
            Argument is sizeof(variable_index_t) bytes long, the variable index. */
        OP_STORE,

        /* Exit the program,
            0 arguments */
        OP_EXIT
    };

    /* The address size that instruction codes use */
    typedef uint32_t address_t;
    /* The index to store variables */
    typedef uint32_t variable_index_t;
    /* The number of call arguments */
    typedef uint8_t call_arguments_t;
    /* Size of constant pool */
    typedef uint32_t constant_index_t;

    typedef std::vector<uint8_t> bytecode_t;
    class Chunk {
        private:
            bytecode_t code = bytecode_t();

            /* Read the value, with the first byte starting at the specified index  */
            template<typename read_type>
            read_type read_value(uint current_byte_index) const {
                #ifdef _SGCPP_SYSTEM_IS_BIG_ENDIAN
                #else
                union {
                    read_type data;
                    uint8_t code[sizeof(read_type)];
                } payload;

                for (uint_fast8_t byte_index = 0; byte_index < sizeof(read_type); byte_index += 1) {
                    payload.code[byte_index] = this->read_byte(current_byte_index + byte_index);
                }

                return payload.data;
                #endif
            }

            /* Insert a value into the code, with the first byte starting at the specified index */
            template<typename insert_type>
            void insert_value(uint index, insert_type data) {
                #ifdef DEBUG
                /* Can't insert into an invalid index */
                assert(this->code.size() >= index + sizeof(insert_type));
                #endif

                union {
                    insert_type payload;
                    /* Don't take for granted the uint32_t. Certain architectures (i.e., Arduino), may not
                        have 4-byte integers. */
                    uint8_t bytes[sizeof(insert_type)];
                } reader;
                reader.payload = data;

                for (uint_fast8_t byte = 0; byte < sizeof(insert_type); byte += 1) {
                    this->code[index + byte] = reader.bytes[byte];
                }
            }

            #ifdef DEBUG
            /* From the current byte index in the code, log the instruction
                and any arguments it has. Then, return the number of bytes we read,
                starting from the current byte, in order to log it. */
            int print_instruction(uint current_byte_index, const Runtime *runtime);
            #endif
        public:
            /* Push an enum into the code.
                Will push only the first byte of the enum value. */
            template<typename enum_t>
            void push_small_enum(enum_t value) {
                this->code.push_back(get_bottom_byte<enum_t>(value));
            }
            /* Push necessary enum functions. Separate them, because if
                they eventually grow to over one byte, we'll need to push
                more than one byte. */
            void push_opcode(OpCode code);           
            void push_bin_op_type(Operations::BinOpType type);
            void push_unary_op_type(Operations::UnaryOpType type);

            template<typename push_type>
            void push_value(push_type data) {
                union {
                    push_type payload;
                    uint8_t bytes[sizeof(push_type)];
                } reader;
                reader.payload = data;

                for (uint_fast8_t byte = 0; byte < sizeof(push_type); byte += 1) {
                    this->code.push_back(reader.bytes[byte]);
                }
            }
            

            /* Push helpers that push the type in individual bytes to the chunk. */
            inline void push_uint32(uint32_t data) {
                this->push_value<uint32_t>(data);
            };
            inline void push_number_value(Values::number_t data) {
                this->push_value<Values::number_t>(data);
            };
            inline void push_address(address_t address) {
                this->push_value<address_t>(address);
            }

            /* Read helpers, read a value of the specified type,
                with the first byte starting at the specified index  */
            uint8_t read_byte(uint index) const;
            inline uint32_t read_uint32(uint current_byte_index) const {
                return this->read_value<uint32_t>(current_byte_index);
            };
            inline Values::number_t read_number_value(uint current_byte_index) const {
                return this->read_value<Values::number_t>(current_byte_index);
            };
            inline address_t read_address(uint current_byte_index) const {
                return this->read_value<address_t>(current_byte_index);
            }

            inline void insert_address(uint index, address_t data) {
                this->insert_value<address_t>(index, data);
            };

            /* Get the current number of instructions
                Made public so that the compiler can reserve code space, and then use the instruction count
                to know where exactly to insert the value later */
            inline size_t code_byte_count() const { return this->code.size(); };

            #ifdef DEBUG
            /* Log representation of bytecode to console */
            void print_code(const Runtime *runtime);
            #endif
    };
};

#endif
