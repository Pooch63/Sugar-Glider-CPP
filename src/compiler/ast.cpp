#include "ast.hpp"
#include "../globals.hpp"

#ifdef DEBUG
#include <cassert>
#endif

using namespace AST;

bool AST::node_is_expression(NodeType type) {
    switch (type) {
        case NODE_BINOP:
        case NODE_UNARYOP:
        case NODE_STRING:
        case NODE_NUMBER:
        case NODE_TRUE:
        case NODE_FALSE:
        case NODE_TERNARY_OP:
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
            return true;
        default:
            return false;
    }
};
bool AST::node_may_be_function(NodeType type) {
    switch (type) {
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
            return true;
        default:
            return false;
    }
}

const char* AST::node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_BINOP:
            return "binary op";
        case NODE_UNARYOP:
            return "unary op";
        case NODE_STRING:
            return "string";
        case NODE_NUMBER:
            return "number";
        case NODE_TRUE:
            return "true";
        case NODE_FALSE:
            return "false";
        case NODE_TERNARY_OP:
            return "ternary op";
        case NODE_VAR_DEFINITION:
            return "variable definition";
        case NODE_VAR_VALUE:
            return "variable value";
        case NODE_VAR_ASSIGNMENT:
            return "variable assignment";
        case NODE_IF:
            return "if statement";
        case NODE_WHILE:
            return "while loop";
        case NODE_BREAK:
            return "break statement";
        case NODE_CONTINUE:
            return "continue statement";
        case NODE_FUNCTION_CALL:
            return "function call";
        case NODE_BODY:
            return "body";
        default:
            #ifdef DEBUG
            assert(false && "unknown AST node type");
            #endif
            return "internal error -- unknown node type. please contact developers";
    }
};


Node::Node(NodeType type) : node_type(type) {};
Node::Node(NodeType type, TokenPosition position) : node_type(type), position(position) {};

String* Node::as_string() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_STRING);
    #endif
    return dynamic_cast<String*>(this);
}
Number* Node::as_number() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_NUMBER);
    #endif
    return dynamic_cast<Number*>(this);
}
BinOp* Node::as_bin_op() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_BINOP);
    #endif
    return dynamic_cast<BinOp*>(this);
}
UnaryOp* Node::as_unary_op() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_UNARYOP);
    #endif
    return dynamic_cast<UnaryOp*>(this);
}
TernaryOp* Node::as_ternary_op() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_TERNARY_OP);
    #endif
    return dynamic_cast<TernaryOp*>(this);
}
VarDefinition* Node::as_variable_definition() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_VAR_DEFINITION);
    #endif
    return dynamic_cast<VarDefinition*>(this);
}
VarValue* Node::as_variable_value() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_VAR_VALUE);
    #endif
    return dynamic_cast<VarValue*>(this);
};
VarAssignment* Node::as_variable_assignment() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_VAR_ASSIGNMENT);
    #endif
    return dynamic_cast<VarAssignment*>(this);
};
If* Node::as_if_statement() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_IF);
    #endif
    return dynamic_cast<If*>(this);
};
While* Node::as_while_loop() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_WHILE);
    #endif
    return dynamic_cast<While*>(this);
}
Break* Node::as_break_statement() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_BREAK);
    #endif
    return dynamic_cast<Break*>(this);
}
Continue* Node::as_continue_statement() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_CONTINUE);
    #endif
    return dynamic_cast<Continue*>(this);
}
FunctionCall* Node::as_function_call() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_FUNCTION_CALL);
    #endif
    return dynamic_cast<FunctionCall*>(this);
}
Function* Node::as_function() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_FUNCTION_DEFINITION);
    #endif
    return dynamic_cast<Function*>(this);
}
Body* Node::as_body() {
    #ifdef DEBUG_ASSERT
    assert(this->node_type == NodeType::NODE_BODY);
    #endif
    return dynamic_cast<Body*>(this);
}

String::String(std::string* str, TokenPosition pos) :
    Node(NodeType::NODE_STRING, pos), str(str) {};
String::~String() {
    if (!this->reserve_string) delete this->str;
}

Number::Number(Values::number_t number) :
    Node(NodeType::NODE_NUMBER)
{
    this->number = number;
}

BinOp::BinOp(Operations::BinOpType type, Node* left, Node* right) :
    Node(NodeType::NODE_BINOP),
    type(type), left(left), right(right) {};
BinOp::~BinOp() {
    if (this->left != nullptr) delete this->left;
    if (this->right != nullptr) delete this->right;
}

UnaryOp::UnaryOp(Operations::UnaryOpType type, Node* argument) :
    Node(NodeType::NODE_UNARYOP),
    type(type), argument(argument) {};
UnaryOp::~UnaryOp() {
    if (this->argument != nullptr) delete this->argument;
}

TernaryOp::TernaryOp(Node* condition, Node* true_value, Node* false_value) :
    Node(NodeType::NODE_TERNARY_OP),
    condition(condition), true_value(true_value), false_value(false_value) {};
TernaryOp::~TernaryOp() {
    if (this->condition != nullptr) delete this->condition;
    if (this->true_value != nullptr) delete this->true_value;
    if (this->false_value != nullptr) delete this->false_value;
}

VarDefinition::VarDefinition(Intermediate::VariableType variable_type, std::string* name, Node* value, TokenPosition position) :
    Node(NodeType::NODE_VAR_DEFINITION, position),
    name(name), value(value), variable_type(variable_type) {};
VarDefinition::~VarDefinition() {
    /* If there is a bug with the name being freed, START HERE. The runtime doesn't need it... for now. */
    if (this->name != nullptr) delete this->name;
    if (this->value != nullptr) delete this->value;
}

VarValue::VarValue(std::string* name, TokenPosition pos) :
    Node(NodeType::NODE_VAR_VALUE, pos),
    name(name) {};
VarValue::~VarValue() {
    if (this->name != nullptr) delete this->name;
}

True::True() : Node(NodeType::NODE_TRUE) {};

False::False() : Node(NodeType::NODE_FALSE) {};

If::If(AST::Node* condition, AST::Node* block) :
    Node(NodeType::NODE_IF),
    condition(condition), block(block) {};
If::~If() {
    if (this->condition != nullptr) delete this->condition;
    if (this->block != nullptr) delete this->block;
}
While::While(AST::Node* condition, AST::Node* block) :
    Node(NodeType::NODE_WHILE),
    condition(condition), block(block) {};
While::~While() {
    if (this->condition != nullptr) delete this->condition;
    if (this->block != nullptr) delete this->block;
}

Break::Break(TokenPosition position) : Node(NodeType::NODE_BREAK, position) {};
Continue::Continue(TokenPosition position) : Node(NodeType::NODE_CONTINUE, position) {};

FunctionCall::FunctionCall(Node* function) : Node(NodeType::NODE_FUNCTION_CALL), function(function) {};
FunctionCall::~FunctionCall() {
    if (this->function != nullptr) delete this->function;
    for (Node* argument : this->arguments) {
        if (argument != nullptr) delete argument;
    }
}

void FunctionCall::add_argument(Node* argument) {
    #ifdef DEBUG
    assert(node_is_expression(argument->get_type()));
    #endif

    this->arguments.push_back(argument);
}

Function::Function(std::string* name, TokenPosition name_position) :
    Node(NodeType::NODE_FUNCTION_DEFINITION, name_position),
    name(name) {};

void Function::add_argument(std::string* argument) {
    this->arguments.push_back(argument);
}
void Function::set_body(Node* body) {
    this->function_body = body;
}

Function::~Function() {
    if (this->name != nullptr) delete name;
    if (this->function_body != nullptr) delete this->function_body;

    for (std::string* argument : this->arguments) {
        if (argument != nullptr) delete argument;
    }
}

Body::Body() : Node(NodeType::NODE_BODY) {};

void Body::add_statement(Node* statement) {
    this->statements.push_back(statement);
};
void Body::reject_scope() {
    this->should_create_scope = false;
}

Body::~Body() {
    for (Node* statement : this->statements) {
        if (statement != nullptr) delete statement;
    }
}

VarAssignment::VarAssignment(Node* variable, Node* value, TokenPosition position) :
    Node(NodeType::NODE_VAR_ASSIGNMENT, position),
    variable(variable), value(value) {};
VarAssignment::~VarAssignment() {
    if (this->variable != nullptr) delete this->variable;
    if (this->value != nullptr) delete this->value;
}