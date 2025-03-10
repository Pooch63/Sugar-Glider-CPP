#ifndef _SGCPP_COMPILER_CPP
#define _SGCPP_COMPILER_CPP

#include "ast.hpp"
#include "../errors.hpp"
#include "../ir/intermediate.hpp"
#include "scopes.hpp"

class Compiler {
    private:
        bool error = false;
        int function_index = Intermediate::global_function_ind;

        Intermediate::LabelIR &ir;
        /* A reference to the main chunk we'll compiling into at the moment */
        Intermediate::Block *main_block;
        Output &output;
        Scopes::ScopeManager scopes = Scopes::ScopeManager();

        /* Try to get variable info from name. Return whether or not it was successful.
            Error if there was an error. */
        bool get_variable_info(AST::VarValue* variable, Intermediate::Variable *&info);

        /* All the compilation functions for specific nodes */
        void compile_array(AST::Array* node);
        void compile_array_index(AST::ArrayIndex* node);
        void compile_dot(AST::Dot *node);
        void compile_string(AST::String* node);
        void compile_number(AST::Number* node);
        void compile_null_value();
        void compile_true_value();
        void compile_false_value();
        void compile_bin_op(AST::BinOp* node);
        void compile_unary_op(AST::UnaryOp* node);
        void compile_ternary_op(AST::TernaryOp* node);
        void compile_variable_definition(AST::VarDefinition* node);
        void compile_variable_value(AST::VarValue* node);
        void compile_variable_assignment(AST::VarAssignment* node);

        void compile_if_statement(AST::If* node);
        void compile_while_loop(AST::While* node);
        void compile_break_statement(AST::Break* node);
        void compile_continue_statement(AST::Continue* node);

        void compile_function_call(AST::FunctionCall* node);
        void compile_function_definition(AST::Function* node);
        void compile_return_statement(AST::Return* node);

        void compile_body(AST::Body* body);

        /* Compile a node into the chunk */
        void compile_node(AST::Node* node);
    public:
        Compiler(Intermediate::LabelIR& ir, Output &output);

        /* Compile the whole program, and then add an exit instruction.
            Returns true if the program was compiled successfully. */
        bool compile(AST::Node* node);
};

#endif