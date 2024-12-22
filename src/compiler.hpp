#ifndef _SGCPP_COMPILER_CPP
#define _SGCPP_COMPILER_CPP

#include "ast.hpp"
#include "ir/intermediate.hpp"

class Compiler {
    private:
        /* A reference to the main chunk we'll compiling into at the moment */
        Intermediate::Block& main_chunk;

        /* All the compilation functions for specific nodes */
        void compile_number(AST::Number* node);
        void compile_bin_op(AST::BinOp* node);
        void compile_unary_op(AST::UnaryOp* node);
        void compile_ternary_op(AST::TernaryOp* node);
        void compile_variable_definition(AST::VarDefinition* node);
        void compile_variable_value(AST::VarValue* node);

        /* Compile a node into the chunk */
        void compile_node(AST::Node* node);
    public:
        Compiler(Intermediate::Block& main_chunk);

        /* Compile the whole program, and then add an exit instruction. */
        void compile(AST::Node* node);
};

#endif