#ifndef _SGCPP_COMPILER_CPP
#define _SGCPP_COMPILER_CPP

#include "ast.hpp"
#include "errors.hpp"
#include "ir/intermediate.hpp"
#include "scopes.hpp"

class Compiler {
    private:
        bool error = false;

        /* A reference to the main chunk we'll compiling into at the moment */
        Intermediate::Block& main_chunk;
        Output &output;
        Scopes::ScopeManager scopes = Scopes::ScopeManager();

        /* All the compilation functions for specific nodes */
        void compile_number(AST::Number* node);
        void compile_true_value();
        void compile_false_value();
        void compile_bin_op(AST::BinOp* node);
        void compile_unary_op(AST::UnaryOp* node);
        void compile_ternary_op(AST::TernaryOp* node);
        void compile_variable_definition(AST::VarDefinition* node);
        void compile_variable_value(AST::VarValue* node);

        void compile_while_loop(AST::While* node);

        void compile_body(AST::Body* body);

        /* Compile a node into the chunk */
        void compile_node(AST::Node* node);
    public:
        Compiler(Intermediate::Block& main_chunk, Output &output);

        /* Compile the whole program, and then add an exit instruction.
            Returns true if the program was compiled successfully. */
        bool compile(AST::Node* node);
};

#endif