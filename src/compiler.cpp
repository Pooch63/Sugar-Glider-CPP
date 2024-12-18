#include "bytecode.hpp"
#include "compiler.hpp"

using namespace Instruction;

Compiler::Compiler(Instruction::Chunk& chunk) : main_chunk(chunk) {};

void Compiler::compile_number(AST::Number* node) {
    this->main_chunk.push_opcode(Instruction::OpCode::OP_NUMBER);
    /* Push the number */
    this->main_chunk.push_number_value(node->get_number());
}
void Compiler::compile_bin_op(AST::BinOp* node) {
    /* Push the arguments. First the left operand, and then the right one. */
    this->compile_node(node->get_left());
    this->compile_node(node->get_right());

    /* Now, push the operation */
    this->main_chunk.push_opcode(OpCode::OP_BIN);
    this->main_chunk.push_bin_op_type(node->get_type());
}
void Compiler::compile_unary_op(AST::UnaryOp* node) {
    /* Push the argument. */
    this->compile_node(node->get_argument());

    /* Now, push the operation */
    this->main_chunk.push_opcode(OpCode::OP_UNARY);
    this->main_chunk.push_unary_op_type(node->get_type());
}
void Compiler::compile_ternary_op(AST::TernaryOp* node) {
    /* Compile condition. */
    this->compile_node(node->get_condition());

    /* If the condition is false, jump to the false value */
    this->main_chunk.push_opcode(OpCode::OP_POP_JIZ);
    /* Reserve space for the address to jump to.
        We'll insert it after compiling the true value. */
    size_t false_jump_arg_start = this->main_chunk.code_byte_count();
    this->main_chunk.push_uint32(0);

    /* Compile value if it's true. */
    this->compile_node(node->get_true_value());

    /* After executing the true value, jump to the end of the block. */
    this->main_chunk.push_opcode(OpCode::OP_GOTO);
    /* Reserve space for the address to jump to.
        We'll insert it after compiling the false value. */
    size_t ending_true_goto_arg_start = this->main_chunk.code_byte_count();
    this->main_chunk.push_uint32(0);

    /* Insert the jump argument if the condition was false.
        Add 1 so that */
    uint32_t false_jump_start = this->main_chunk.code_byte_count();
    this->main_chunk.insert_address(false_jump_arg_start, false_jump_start);

    /* Compile value if it's false. */
    this->compile_node(node->get_false_value());

    /* Insert the address to jump to after completing the if_true block */
    size_t block_end = this->main_chunk.code_byte_count();
    this->main_chunk.insert_address(ending_true_goto_arg_start, block_end);
}

void Compiler::compile_node(AST::Node* node) {
    switch (node->get_type()) {
        /* Push a number onto the stack */
        case AST::NodeType::NODE_NUMBER:
            this->compile_number(node->as_number());
            break;
        case AST::NodeType::NODE_BINOP:
            this->compile_bin_op(node->as_bin_op());
            break;
        case AST::NodeType::NODE_UNARYOP:
            this->compile_unary_op(node->as_unary_op());
            break;
        case AST::NodeType::NODE_TERNARY_OP:
            this->compile_ternary_op(node->as_ternary_op());
            break;
    }
}
void Compiler::compile(AST::Node* node) {
    this->compile_node(node);
    this->main_chunk.push_opcode(OpCode::OP_EXIT);
}