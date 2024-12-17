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
    this->compile(node->get_left());
    this->compile(node->get_right());

    /* Now, push the operation */
    this->main_chunk.push_opcode(OpCode::OP_BIN);
    this->main_chunk.push_bin_op_type(node->get_type());
}
void Compiler::compile_unary_op(AST::UnaryOp* node) {
    /* Push the argument. */
    this->compile(node->get_argument());

    /* Now, push the operation */
    this->main_chunk.push_opcode(OpCode::OP_UNARY);
    this->main_chunk.push_unary_op_type(node->get_type());
}

void Compiler::compile(AST::Node* node) {
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
    }
}