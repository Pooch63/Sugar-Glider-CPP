#include "ast.hpp"
#include "globals.hpp"

#ifdef DEBUG
#include <cassert>
#endif

using namespace AST;

bool AST::node_is_expression(NodeType type) {
    switch (type) {
        case NODE_BINOP:
        case NODE_UNARYOP:
        case NODE_NUMBER:
        case NODE_TRUE:
        case NODE_FALSE:
        case NODE_TERNARY_OP:
        case NODE_VAR_VALUE:
            return true;
        default:
            return false;
    }
};

Node::Node(NodeType type) : node_type(type) {};
Node::Node(NodeType type, TokenPosition position) : node_type(type), position(position) {};

Number* Node::as_number() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_NUMBER);
    #endif
    return dynamic_cast<Number*>(this);
}
BinOp* Node::as_bin_op() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_BINOP);
    #endif
    return dynamic_cast<BinOp*>(this);
}
UnaryOp* Node::as_unary_op() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_UNARYOP);
    #endif
    return dynamic_cast<UnaryOp*>(this);
}
TernaryOp* Node::as_ternary_op() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_TERNARY_OP);
    #endif
    return dynamic_cast<TernaryOp*>(this);
}
VarDefinition* Node::as_variable_definition() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_VAR_DEFINITION);
    #endif
    return dynamic_cast<VarDefinition*>(this);
}
VarValue* Node::as_variable_value() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_VAR_VALUE);
    #endif
    return dynamic_cast<VarValue*>(this);
};
VarAssignment* Node::as_variable_assignment() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_VAR_ASSIGNMENT);
    #endif
    return dynamic_cast<VarAssignment*>(this);
};
While* Node::as_while_loop() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_WHILE);
    #endif
    return dynamic_cast<While*>(this);
}
Body* Node::as_body() {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_BODY);
    #endif
    return dynamic_cast<Body*>(this);
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

VarDefinition::VarDefinition(Scopes::VariableType variable_type, std::string* name, Node* value, TokenPosition position) :
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

While::While(AST::Node* condition, AST::Node* block) :
    Node(NodeType::NODE_WHILE),
    condition(condition), block(block) {};
While::~While() {
    if (this->condition != nullptr) delete this->condition;
    if (this->block != nullptr) delete this->block;
}


Body::Body() : Node(NodeType::NODE_BODY) {};
void Body::add_statement(Node* statement) {
    this->statements.push_back(statement);
};
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