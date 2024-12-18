#ifndef _SGCPP_AST_HPP
#define _SGCPP_AST_HPP

#include "lexer.hpp"
#include "operations.hpp"

namespace AST {
    enum NodeType {
        NODE_NUMBER,

        NODE_BINOP,
        NODE_UNARYOP,
        NODE_TERNARY_OP
    };

    class Number;
    class BinOp;
    class UnaryOp;
    class TernaryOp;

    /* A collection of pointers to all the possible nodes
        that the node could be wrapping. While this COULD be
        a void* instead, this is preferred to avoid memory bugs. */
    union node_wrapper_t {
        Number* number;
        BinOp* bin_op;
        UnaryOp* unary_op;
        TernaryOp* ternary_op;
    };

    class Node {
        private:
            NodeType node_type;
            node_wrapper_t node;

        public:
            Node(NodeType type, node_wrapper_t node);

            inline NodeType get_type() const { return this->node_type; };

            /* Return pointers to nodes depending on the type you specify.
                The wrapper type MUST be the same as the node type. */
            Number* as_number() const;
            BinOp* as_bin_op() const;
            UnaryOp* as_unary_op() const;
            TernaryOp* as_ternary_op() const;

            /* Frees the node payload depending on its type. */
            ~Node();
    };

    class Number : public Node {
        private:
            Values::number_t number;
        public:
            Number(Values::number_t number);

            inline Values::number_t get_number() const { return this->number; };
    };
    class BinOp : public Node {
        private:
            Operations::BinOpType type;
            Node* left;
            Node* right;
        public:
            BinOp(Operations::BinOpType type, Node* left, Node* right);

            inline Operations::BinOpType get_type() const { return this->type; };
            inline Node* get_left() const { return this->left; };
            inline Node* get_right() const { return this->right; };

            void free();
    };
    class UnaryOp : public Node {
        private:
            Operations::UnaryOpType type;
            Node* argument;
        public:
            UnaryOp(Operations::UnaryOpType type, Node* argument);

            inline Operations::UnaryOpType get_type() const { return this->type; }
            inline Node* get_argument() const { return this->argument; }

            void free();
    };
    class TernaryOp : public Node {
        private:
            Node* condition;
            Node* true_value;
            Node* false_value;
        public:
            TernaryOp(Node* condition, Node* true_value, Node* false_value);

            inline Node* get_condition() const { return this->condition; }
            inline Node* get_true_value() const { return this->true_value; }
            inline Node* get_false_value() const { return this->false_value; }

            void free();
    };
}

#endif