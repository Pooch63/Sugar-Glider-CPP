#include "compiler.hpp"
#include "../globals.hpp"
#include "../ir/bytecode.hpp"

#ifdef DEBUG
#include <cassert>
#endif

#include <string>

using namespace Bytecode;
using Intermediate::ir_instruction_arg_t, Intermediate::label_index_t, Intermediate::Variable;
using Scopes::ScopeType;

Compiler::Compiler(Intermediate::LabelIR& block, Output &output) : ir(block), main_block(block.get_main()->get_block()), output(output) {
    this->scopes.new_scope(ScopeType::NORMAL);
};

void Compiler::compile_array(AST::Array* node) {
    for (AST::Node *element : *node) {
        this->compile_node(element);
    }
    this->main_block->add_instruction(Intermediate::Instruction(
        Intermediate::INSTR_MAKE_ARRAY, node->element_count() ));
}
void Compiler::compile_array_index(AST::ArrayIndex* node) {
    this->compile_node(node->get_array());
    this->compile_node(node->get_index());

    if (node->is_value_get()) {
        this->main_block->add_instruction(
            Intermediate::Instruction(Intermediate::INSTR_GET_ARRAY_VALUE));
    }
    else {
        this->compile_node(node->get_value());
        this->main_block->add_instruction(
            Intermediate::Instruction(Intermediate::INSTR_SET_ARRAY_VALUE));
    }
}
void Compiler::compile_string(AST::String* node) {
    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_STRING,
            new std::string(*node->get_string()) ));
}
void Compiler::compile_number(AST::Number* node) {
    this->main_block->add_instruction(Intermediate::Instruction(node->get_number()));
}
void Compiler::compile_null_value() {
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_NULL));
}
void Compiler::compile_true_value() {
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_TRUE));
}
void Compiler::compile_false_value() {
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_FALSE));
}

void Compiler::compile_bin_op(AST::BinOp* node) {
    // /* Push the arguments. First the left operand, and then the right one. */
    this->compile_node(node->get_left());
    this->compile_node(node->get_right());

    // /* Now, push the operation */
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_BIN_OP, node->get_type()));
}
void Compiler::compile_unary_op(AST::UnaryOp* node) {
    /* Push the argument. */
    this->compile_node(node->get_argument());

    /* Now, push the operation */
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_UNARY_OP, node->get_type()));
}
void Compiler::compile_ternary_op(AST::TernaryOp* node) {
//     /* Compile condition. */
    this->compile_node(node->get_condition());

    label_index_t* end = this->main_block->gen_label_name();

    /* Jump past true value if condition was false */
    label_index_t* false_label = this->main_block->gen_label_name();
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP_JIZ, false_label));

    /* Compile the value if true */
    this->compile_node(node->get_true_value());

    /* Jump to the end. */
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, end));

    /* Compile the false value */
    // Add a label for it
    this->main_block->new_label(false_label);
    this->compile_node(node->get_false_value());

    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, end));

    /* And now, exit the whole control flow if false */
    this->main_block->new_label(end);
}

bool Compiler::get_variable_info(AST::VarValue* variable, Intermediate::Variable *&info) {
    bool var_found = this->scopes.get_variable(variable->get_name(), info);

    /* Make sure the variable exists */
    if (!var_found) {
        std::string name;
        truncate_string(name, 30, *variable->get_name());

        char error[100];
        snprintf(error, 100, "Variable \"%s\" does not exist.", name.c_str());
        this->output.error(variable->get_position(), error, Errors::COMPILE_ERROR);
        this->error = true;
        return false;
    }
    return true;
};

void Compiler::compile_variable_definition(AST::VarDefinition* def) {
    this->compile_node(def->get_value());

    /* Make sure the variable can be declared. */
    if (this->scopes.last_scope_has_variable(def->get_name())) {
        char error[100];
        snprintf(error, 100, "Variable \"%s\" has already been declared in this scope.", def->get_name()->c_str());
        this->output.error(def->get_position(), error, Errors::COMPILE_ERROR);
        this->error = true;
        return;
    }

    /* Add the variable to scopes */
    Intermediate::Variable *variable = this->scopes.add_variable(
        def->get_name(),
        this->scopes.add_variable_headers(def->get_basic_variable_type()), this->function_index);

    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_STORE,
            variable ));
}
void Compiler::compile_variable_value(AST::VarValue* node) {
    Intermediate::Variable *var_info;
    
    if (!this->get_variable_info(node, var_info)) return;

    this->main_block->begin();
    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_LOAD,
            var_info ));

    /* Close variable if necessary */
    if (var_info->function_ind != this->function_index && !var_info->in_topmost_scope()) {
        var_info->close();
        this->output.error(
            node->get_position(),
            "Cannot close variables (cannot access variables in one function from within a nested function). This functionality is not yet supported.",
            Errors::COMPILE_ERROR);
        this->error = true;
    }
};
void Compiler::compile_variable_assignment(AST::VarAssignment* node) {
    this->compile_node(node->get_value());

    AST::Node* variable = node->get_variable();

    switch (variable->get_type()) {
        case AST::NODE_VAR_VALUE:
        {
            Intermediate::Variable *var_info;
            
            if (!this->get_variable_info(node->get_variable()->as_variable_value(), var_info)) return;

            using namespace Intermediate;
            VariableType type = var_info->type;
            if (
                type == GLOBAL_CONSTANT || type == FUNCTION_CONSTANT ||
                type == CLOSED_CONSTANT || type == NATIVE
            ) {
                std::string error_message;
                if (type == NATIVE) error_message = "Cannot reassign native variable \"";
                else error_message = "Cannot assign a value to constant variable \"";

                error_message += *var_info->name;
                error_message += '"';
                this->output.error(node->get_position(), error_message, Errors::COMPILE_ERROR);
                this->error = true;
            }

            this->main_block->add_instruction(
                Intermediate::Instruction(
                    Intermediate::INSTR_STORE,
                    var_info ));

            /* In expressions like var g = x = 1, we need to load the value of the variable
                after calculating it. */
            this->main_block->add_instruction(
                Intermediate::Instruction(
                    Intermediate::INSTR_LOAD,
                    var_info ));
        }
            break;
        /* Unknown variable to set */
        default:
            throw sg_assert_error("Parse error: Invalid variable node type to compile");
    }
};

void Compiler::compile_if_statement(AST::If* node) {
    // The label following the if-elseif-else chain
    label_index_t* end_label = this->main_block->gen_label_name();

    this->main_block->new_label();
    this->compile_node(node->get_condition());

    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_POP_JIZ,
            end_label ) );

    this->compile_node(node->get_block());
    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_GOTO,
            end_label ) );

    this->main_block->new_label(end_label);
}
void Compiler::compile_while_loop(AST::While* node) {
    /* First, compile the condition. We have to jump back to this
        at the end of the loop, so make a new label. */
    label_index_t* condition = this->main_block->new_label();

    // The index of the ending label
    label_index_t* end = this->main_block->gen_label_name();

    this->compile_node(node->get_condition());
    /* If the condition is false, jump to the end */
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP_JIZ, end));

    /* Now put the body in a new label. */
    this->main_block->new_label();

    this->scopes.new_scope(ScopeType::LOOP, condition, end);

    // Block a new scope from being created since we just created a loop one
    if (node->get_type() == AST::NODE_BODY) node->get_block()->as_body()->reject_scope();

    this->compile_node(node->get_block());
    // Add a goto command to evaluate the condition afterway
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, condition));

    this->scopes.pop_scope();

    // Create the ending label name
    this->main_block->new_label(end);
}
void Compiler::compile_break_statement(AST::Break* node) {
    label_index_t* loop_end = this->scopes.get_loop_end();
    if (loop_end == nullptr) {
        this->output.error(node->get_position(), "A break statement may only appear in a loop.", Errors::COMPILE_ERROR);
        this->error = true;
        return;
    }

    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_GOTO,
            loop_end ) );
}
void Compiler::compile_continue_statement(AST::Continue* node) {
    label_index_t* loop_start = this->scopes.get_loop_condition_label();
    if (loop_start == nullptr) {
        this->output.error(node->get_position(), "A continue statement may only appear in a loop.", Errors::COMPILE_ERROR);
        this->error = true;
        return;
    }

    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_GOTO,
            loop_start ) );
}

void Compiler::compile_function_call(AST::FunctionCall* node) {
    for (auto argument : *node) {
        this->compile_node(argument);
    }
    this->compile_node(node->get_function());
    this->main_block->add_instruction(
        Intermediate::Instruction(
            Intermediate::INSTR_CALL,
            node->argument_count() ));
}
void Compiler::compile_function_definition(AST::Function* node) {
    /* Make sure the variable can be declared. */
    if (this->scopes.last_scope_has_variable(node->get_name())) {
        char error[100];
        std::string name;
        truncate_string(name, 30, *node->get_name());
        snprintf(error, 100, "Function \"%s\" cannot have the same name as a variable in the same scope.", name.c_str());
        this->output.error(node->get_position(), error, Errors::COMPILE_ERROR);
        this->error = true;
    }

    this->main_block->add_instruction(Intermediate::Instruction(
            Intermediate::INSTR_GET_FUNCTION_REFERENCE,
            static_cast<uint>(this->ir.last_function_index()) + 1));
    // Add top level function
    if (!this->scopes.in_function()) {
        /* Add the variable to scopes */
        Intermediate::Variable *variable = this->scopes.add_variable(node->get_name(), Intermediate::GLOBAL_CONSTANT, this->function_index);
        this->main_block->add_instruction(
            Intermediate::Instruction(
                Intermediate::INSTR_STORE,
                variable ));
    }
    // Add it to function
    else {
        this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_MAKE_FUNCTION));
    }

    Intermediate::Block *old_compile = this->main_block;
    Intermediate::Function *function = this->ir.new_function();

    this->main_block = function->get_block();

    int last_function_index = this->function_index;
    this->function_index = this->ir.last_function_index();

    /* Add function arguments */
    this->scopes.new_scope(Scopes::FUNCTION);

    for (std::string *arg : *node) {
        Intermediate::Variable *var = this->scopes.add_variable(arg, Intermediate::FUNCTION_MUTABLE, this->function_index);
        function->add_argument(var);
    }

    this->compile_node(node->get_body());
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_NULL));
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_RETURN));

    this->scopes.pop_scope();

    this->function_index = last_function_index;
    this->main_block = old_compile;
}
void Compiler::compile_return_statement(AST::Return* node) {
    if (!this->scopes.in_function()) {
        this->output.error(node->get_position(), "Illegal return statement in non-function scope", Errors::COMPILE_ERROR);
        this->error = true;
    }

    this->compile_node(node->get_return_value());
    this->main_block->add_instruction(
        Intermediate::Instruction(Intermediate::INSTR_RETURN)
    );
};

void Compiler::compile_body(AST::Body* body) {
    if (!body->will_create_scope()) scopes.new_scope(ScopeType::NORMAL);

    for (AST::Node* statement : *body) {
        this->compile_node(statement);
        /* If it's an expression, e.g. 4;, then we need the pop the result (in this case,
            the 4 that gets added to the stack.) */
        if (AST::node_is_expression(statement->get_type())) {
            this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_POP));
        }
    }

    if (!body->will_create_scope()) scopes.pop_scope();
}

void Compiler::compile_node(AST::Node* node) {
    /* The parser may create a null node pointer if there was an error
        In that case, we should have halted compilation, but just make sure. */
    #ifdef DEBUG
    assert(node != nullptr);
    #endif

    switch (node->get_type()) {
        /* Push these values onto the stack */
        case AST::NodeType::NODE_ARRAY:
            this->compile_array(node->as_array());
            break;
        case AST::NodeType::NODE_ARRAY_INDEX:
            this->compile_array_index(node->as_array_index());
            break;
        case AST::NodeType::NODE_STRING:
            this->compile_string(node->as_string());
            break;
        case AST::NodeType::NODE_NUMBER:
            this->compile_number(node->as_number());
            break;
        case AST::NodeType::NODE_NULL:
            this->compile_null_value();
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
        case AST::NodeType::NODE_VAR_ASSIGNMENT:
            this->compile_variable_assignment(node->as_variable_assignment());
            break;

        case AST::NodeType::NODE_IF:
            this->compile_if_statement(node->as_if_statement());
            break;
        case AST::NodeType::NODE_WHILE:
            this->compile_while_loop(node->as_while_loop());
            break;
        case AST::NodeType::NODE_BREAK:
            this->compile_break_statement(node->as_break_statement());
            break;
        case AST::NodeType::NODE_CONTINUE:
            this->compile_continue_statement(node->as_continue_statement());
            break;

        case AST::NodeType::NODE_FUNCTION_CALL:
            this->compile_function_call(node->as_function_call());
            break;
        case AST::NodeType::NODE_FUNCTION_DEFINITION:
            this->compile_function_definition(node->as_function());
            break;
        case AST::NodeType::NODE_RETURN:
            this->compile_return_statement(node->as_return_statement());
            break;

        case AST::NodeType::NODE_BODY:
            this->compile_body(node->as_body());
    }
}
bool Compiler::compile(AST::Node* node) {
    this->compile_node(node);
    this->main_block->new_label();
    this->main_block->add_instruction(Intermediate::Instruction(Intermediate::INSTR_EXIT));
    return !this->error;
}
