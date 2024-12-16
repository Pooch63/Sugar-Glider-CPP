#ifndef _SGCPP_COMPILER_CPP
#define _SGCPP_COMPILER_CPP

#include "ast.hpp"
#include "bytecode.hpp"

class Compiler {
    private:
        /* A reference to the main chunk we'll compile into */
        Instruction::Chunk& main_chunk;

        /* All the compilation functions for specific nodes */
        void compile_number(AST::Number* node);
        void compile_bin_op(AST::BinOp* node);
        void compile_unary_op(AST::UnaryOp* node);
    public:
        Compiler(Instruction::Chunk& main_chunk);

        /* Compile a node into the chunk */
        void compile(AST::Node* node);
};

#endif