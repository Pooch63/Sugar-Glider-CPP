#include "ast.hpp"
#include "../globals.hpp"

#ifdef DEBUG
#include <cassert>
#endif

using namespace AST;

bool AST::node_is_expression(NodeType type) {
    switch (type) {
        case NODE_ARRAY:
        case NODE_ARRAY_INDEX:
        case NODE_DOT:
        case NODE_BINOP:
        case NODE_UNARYOP:
        case NODE_STRING:
        case NODE_NUMBER:
        case NODE_TRUE:
        case NODE_FALSE:
        case NODE_TERNARY_OP:
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
        case NODE_FUNCTION_CALL:
            return true;
        default:
            return false;
    }
};
bool AST::node_may_be_function(NodeType type) {
    switch (type) {
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
        case NODE_FUNCTION_CALL:
        case NODE_ARRAY_INDEX:
        case NODE_TERNARY_OP:
        case NODE_DOT:
            return true;
        default:
            return false;
    }
}
bool AST::node_may_be_array(NodeType type) {
    switch (type) {
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
        case NODE_FUNCTION_CALL:
        case NODE_ARRAY_INDEX:
        case NODE_TERNARY_OP:
        case NODE_ARRAY:
        case NODE_DOT:
            return true;
        default:
            return false;
    }
}
bool AST::node_may_be_object(NodeType type) {
    switch (type) {
        case NODE_VAR_VALUE:
        case NODE_VAR_ASSIGNMENT:
        case NODE_FUNCTION_CALL:
        case NODE_ARRAY_INDEX:
        case NODE_TERNARY_OP:
        case NODE_DOT:
            return true;
        default:
            return false;
    }
}

const char* AST::node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_ARRAY:
            return "array";
        case NODE_ARRAY_INDEX:
            return "array index";
        case NODE_DOT:
            return "dot operator";
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

#ifdef DEBUG_ASSERT
#define AST_CAST_DEFINE(type, func_name, node_name) type *Node::func_name() { \
    assert(this->node_type == NodeType::node_name); \
    return dynamic_cast<type*>(this); \
};
#else
#define AST_CAST_DEFINE(type, func_name, node_name) type *Node::func_name() { \
    return dynamic_cast<type*>(this); \
};
#endif
AST_CAST_DEFINE(Array, as_array, NODE_ARRAY)
AST_CAST_DEFINE(ArrayIndex, as_array_index, NODE_ARRAY_INDEX)
AST_CAST_DEFINE(Dot, as_dot, NODE_DOT)
AST_CAST_DEFINE(String, as_string, NODE_STRING)
AST_CAST_DEFINE(Number, as_number, NODE_NUMBER)
AST_CAST_DEFINE(BinOp, as_bin_op, NODE_BINOP)
AST_CAST_DEFINE(UnaryOp, as_unary_op, NODE_UNARYOP)
AST_CAST_DEFINE(TernaryOp, as_ternary_op, NODE_TERNARY_OP)
AST_CAST_DEFINE(VarDefinition, as_variable_definition, NODE_VAR_DEFINITION)
AST_CAST_DEFINE(VarValue, as_variable_value, NODE_VAR_VALUE)
AST_CAST_DEFINE(VarAssignment, as_variable_assignment, NODE_VAR_ASSIGNMENT)
AST_CAST_DEFINE(If, as_if_statement, NODE_IF)
AST_CAST_DEFINE(While, as_while_loop, NODE_WHILE)
AST_CAST_DEFINE(Break, as_break_statement, NODE_BREAK)
AST_CAST_DEFINE(Continue, as_continue_statement, NODE_CONTINUE)
AST_CAST_DEFINE(FunctionCall, as_function_call, NODE_FUNCTION_CALL)
AST_CAST_DEFINE(Function, as_function, NODE_FUNCTION_DEFINITION)
AST_CAST_DEFINE(Return, as_return_statement, NODE_RETURN)
AST_CAST_DEFINE(Body, as_body, NODE_BODY)

Array::Array(): Node(NodeType::NODE_ARRAY) {}
void Array::add_element(Node *value) {
    this->values.push_back(value);
}
Array::~Array() {
    for (Node *value : this->values) {
        delete value;
    }
}

ArrayIndex::ArrayIndex(Node *array, Node *index, Node *value) :
    Node(NodeType::NODE_ARRAY_INDEX), array(array), index(index), value(value) {};
ArrayIndex::~ArrayIndex() {
    if (this->array != nullptr) delete this->array;
    if (this->index != nullptr) delete this->index;
    if (this->value != nullptr) delete this->value;
}

Dot::Dot(AST::Node *left, std::string *property) :
    Node(NodeType::NODE_DOT), left(left), property(property) {};
Dot::~Dot() {
    if (this->left != nullptr) delete this->left;
    if (this->property != nullptr) delete this->property;
}

String::String(std::string* str, TokenPosition pos) :
    Node(NodeType::NODE_STRING, pos), str(str) {};
String::~String() {
    delete this->str;
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

VarDefinition::VarDefinition(Intermediate::VariableType basic_variable_type, std::string* name, Node* value, TokenPosition position) :
    Node(NodeType::NODE_VAR_DEFINITION, position),
    name(name), value(value), basic_variable_type(basic_variable_type) {};
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

Null::Null() : Node(NodeType::NODE_NULL) {};

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

Return::Return(Node *return_value, TokenPosition return_position) :
    Node(NodeType::NODE_RETURN, return_position),
    return_value(return_value) {};
Return::~Return() {
    if (this->return_value != nullptr) delete this->return_value;
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