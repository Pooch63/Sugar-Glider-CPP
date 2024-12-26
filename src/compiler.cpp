#include "ir/bytecode.hpp"
#include "compiler.hpp"
#include "globals.hpp"

#ifdef DEBUG
#include <cassert>
#endif

using namespace Bytecode;
using Intermediate::ir_instruction_arg_t;

Compiler::Compiler(Intermediate::Block& chunk) : main_chunk(chunk) {};

void Compiler::compile_number(AST::Number* node) {
    this->main_chunk.add_instruction(Intermediate::Instruction(node->get_number()));
}
void Compiler::compile_true_value() {
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_TRUE));
}
void Compiler::compile_false_value() {
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_FALSE));
}

void Compiler::compile_bin_op(AST::BinOp* node) {
    // /* Push the arguments. First the left operand, and then the right one. */
    this->compile_node(node->get_left());
    this->compile_node(node->get_right());

    // /* Now, push the operation */
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_BIN_OP, node->get_type()));
}
void Compiler::compile_unary_op(AST::UnaryOp* node) {
    /* Push the argument. */
    this->compile_node(node->get_argument());

    /* Now, push the operation */
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_UNARY_OP, node->get_type()));
}
void Compiler::compile_ternary_op(AST::TernaryOp* node) {
//     /* Compile condition. */
    this->compile_node(node->get_condition());

    /* Get next label */
    uint next = this->main_chunk.label_count();
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP_JIZ, next));

    /* Compile the value if true */
    this->compile_node(node->get_true_value());

    /* Jump to the end. Add 1 to account for the false label. */
    int end = this->main_chunk.label_count() + 1;
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, end));

    /* Compile the false value */
    // Add a label for it
    this->main_chunk.new_label();
    this->compile_node(node->get_false_value());

    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, end));

    /* And now, exit the whole control flow if false */
    this->main_chunk.new_label();
}
void Compiler::compile_variable_definition(AST::VarDefinition* def) {
    this->compile_node(def->get_value());

    this->main_chunk.add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_STORE,
            Intermediate::Variable{ .name = def->get_name() }
        )
    );
}
void Compiler::compile_variable_value(AST::VarValue* node) {
    this->main_chunk.add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_LOAD,
            Intermediate::Variable{ .name = node->get_name() }
        )
    );
};
void Compiler::compile_while_loop(AST::While* node) {
    /* First, compile the condition. We have to jump back to this
        at the end of the loop, so make a new label. */
    int condition_label = this->main_chunk.label_count();
    this->main_chunk.new_label();
    this->compile_node(node->get_condition());
    /* If the condition is false, jump to the end */
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP_JIZ, condition_label + 2));

    /* Now put the body in a new label. */
    this->main_chunk.new_label();
    this->compile_node(node->get_block());
    // Add a goto command to evaluate the condition afterway
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, condition_label));
}

void Compiler::compile_body(AST::Body* body) {
    for (AST::Node* statement : *body) {
        this->compile_node(statement);
        /* If it's an expression, e.g. 4;, then we need the pop the result (in this case,
            the 4 that gets added to the stack.) */
        if (AST::node_is_expression(statement->get_type())) {
            this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP));
        }
    }
}

void Compiler::compile_node(AST::Node* node) {
    /* The parser may create a null node pointer if there was an error
        In that case, we should have halted compilation, but just make sure. */
    #ifdef DEBUG
    assert(node != nullptr);
    #endif

    switch (node->get_type()) {
        /* Push a number onto the stack */
        case AST::NodeType::NODE_NUMBER:
            this->compile_number(node->as_number());
            break;
        case AST::NodeType::NODE_TRUE:
            this->compile_true_value();
            break;
        case AST::NodeType::NODE_FALSE:
            this->compile_false_value();
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

        case AST::NodeType::NODE_VAR_DEFINITION:
            this->compile_variable_definition(node->as_variable_definition());
            break;
        case AST::NodeType::NODE_VAR_VALUE:
            this->compile_variable_value(node->as_variable_value());
            break;

        case AST::NodeType::NODE_WHILE:
            this->compile_while_loop(node->as_while_loop());
            break;

        case AST::NodeType::NODE_BODY:
            this->compile_body(node->as_body());
    }
}
void Compiler::compile(AST::Node* node) {
    this->compile_node(node);
    this->main_chunk.new_label();
    this->main_chunk.add_instruction(Intermediate::Instruction(Intermediate::INSTR_EXIT));
}