#include "ast.hpp"
#include "globals.hpp"

#ifdef DEBUG
#include <cassert>
#endif

using namespace AST;

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

Node::~Node() {
    switch (this->node_type) {
        case NODE_NUMBER:
            break;
        case NODE_BINOP:
            this->node.bin_op->free();
            break;
        case NODE_UNARYOP:
            this->node.unary_op->free();
            break;
    }
};

Number::Number(Value::number_t number) :
    Node(NodeType::NODE_NUMBER, node_wrapper_t{ .number = this })
{
    this->number = number;
}


BinOp::BinOp(Operations::BinOpType type, Node* left, Node* right) :
    Node(NodeType::NODE_BINOP, node_wrapper_t{ .bin_op = this }),
    type(type), left(left), right(right) {};

void BinOp::free() {
    delete this->left;
    delete this->right;
}

UnaryOp::UnaryOp(Operations::UnaryOpType type, Node* argument) :
    Node(NodeType::NODE_UNARYOP, node_wrapper_t{ .unary_op = this }),
    type(type), argument(argument) {};
void UnaryOp::free() {
    delete this->argument;
}

TernaryOp::TernaryOp(Node* condition, Node* true_value, Node* false_value) :
    Node(NodeType::NODE_TERNARY_OP, node_wrapper_t{ .ternary_op = this }),
    condition(condition), true_value(true_value), false_value(false_value) {};
void TernaryOp::free() {
    delete this->condition;
    delete this->true_value;
    delete this->false_value;
}