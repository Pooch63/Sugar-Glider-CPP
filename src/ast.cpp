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

Node::Node(NodeType type, node_wrapper_t node) : node_type(type), node(node) {};

Number* Node::as_number() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_NUMBER);
    #endif
    return this->node.number;
}
BinOp* Node::as_bin_op() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_BINOP);
    #endif
    return this->node.bin_op;
}
UnaryOp* Node::as_unary_op() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_UNARYOP);
    #endif
    return this->node.unary_op;
}
TernaryOp* Node::as_ternary_op() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_TERNARY_OP);
    #endif
    return this->node.ternary_op;
}
VarDefinition* Node::as_variable_definition() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_VAR_DEFINITION);
    #endif
    return this->node.var_definition;
}
VarValue* Node::as_variable_value() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_VAR_VALUE);
    #endif
    return this->node.var_value;
};
While* Node::as_while_loop() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_WHILE);
    #endif
    return this->node.while_node;
}
Body* Node::as_body() const {
    #ifdef DEBUG
    assert(this->node_type == NodeType::NODE_BODY);
    #endif
    return this->node.body;
}

Node::~Node() {
    switch (this->node_type) {
        case NODE_NUMBER:
        case NODE_TRUE:
        case NODE_FALSE:
            break;
        case NODE_BINOP:
            this->node.bin_op->free();
            break;
        case NODE_UNARYOP:
            this->node.unary_op->free();
            break;
        case NODE_TERNARY_OP:
            this->node.ternary_op->free();
            break;
        case NODE_VAR_DEFINITION:
            this->node.var_definition->free();
            break;
        case NODE_VAR_VALUE:
            this->node.var_value->free();
            break;
        case NODE_WHILE:
            this->node.while_node->free();
            break;
        case NODE_BODY:
            this->node.body->free();
            break;
    }
};

Number::Number(Values::number_t number) :
    Node(NodeType::NODE_NUMBER, node_wrapper_t{ .number = this })
{
    this->number = number;
}

BinOp::BinOp(Operations::BinOpType type, Node* left, Node* right) :
    Node(NodeType::NODE_BINOP, node_wrapper_t{ .bin_op = this }),
    type(type), left(left), right(right) {};
void BinOp::free() {
    if (this->left != nullptr) delete this->left;
    if (this->right != nullptr) delete this->right;
}

UnaryOp::UnaryOp(Operations::UnaryOpType type, Node* argument) :
    Node(NodeType::NODE_UNARYOP, node_wrapper_t{ .unary_op = this }),
    type(type), argument(argument) {};
void UnaryOp::free() {
    if (this->argument != nullptr) delete this->argument;
}

TernaryOp::TernaryOp(Node* condition, Node* true_value, Node* false_value) :
    Node(NodeType::NODE_TERNARY_OP, node_wrapper_t{ .ternary_op = this }),
    condition(condition), true_value(true_value), false_value(false_value) {};
void TernaryOp::free() {
    if (this->condition != nullptr) delete this->condition;
    if (this->true_value != nullptr) delete this->true_value;
    if (this->false_value != nullptr) delete this->false_value;
}

VarDefinition::VarDefinition(std::string* name, Node* value) :
    Node(NodeType::NODE_VAR_DEFINITION, node_wrapper_t{ .var_definition = this }),
    name(name), value(value) {};
void VarDefinition::free() {
    /* If there is a bug with the name being freed, START HERE. The runtime doesn't need it... for now. */
    if (this->name != nullptr) delete this->name;
    if (this->value != nullptr) delete this->value;
}

VarValue::VarValue(std::string* name) :
    Node(NodeType::NODE_VAR_VALUE, node_wrapper_t{ .var_value = this }),
    name(name) {};
void VarValue::free() {
    if (this->name != nullptr) delete this->name;
}

True::True() : Node(NodeType::NODE_TRUE, node_wrapper_t{ .true_value = this }) {};

False::False() : Node(NodeType::NODE_FALSE, node_wrapper_t{ .false_value = this }) {};

While::While(AST::Node* condition, AST::Node* block) :
    Node(NodeType::NODE_WHILE, node_wrapper_t{ .while_node = this }),
    condition(condition), block(block) {};
void While::free() {
    if (this->condition != nullptr) delete this->condition;
    if (this->block != nullptr) delete this->block;
}


Body::Body() : Node(NodeType::NODE_BODY, node_wrapper_t{ .body = this }) {};
void Body::add_statement(Node* statement) {
    this->statements.push_back(statement);
};
void Body::free() {
    for (Node* statement : this->statements) {
        if (statement != nullptr) delete statement;
    }
}