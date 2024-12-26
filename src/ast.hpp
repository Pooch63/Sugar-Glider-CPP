#ifndef _SGCPP_AST_HPP
#define _SGCPP_AST_HPP

#include "lexer.hpp"
#include "operations.hpp"
#include "scopes.hpp"

namespace AST {
    enum NodeType {
        NODE_NUMBER,
        NODE_TRUE,
        NODE_FALSE,

        NODE_BINOP,
        NODE_UNARYOP,
        NODE_TERNARY_OP,

        NODE_VAR_DEFINITION,
        NODE_VAR_VALUE,
        NODE_VAR_ASSIGNMENT,

        NODE_WHILE,

        NODE_BODY
    };

    bool node_is_expression(NodeType type);

    class Number;
    class True;
    class False;
    class BinOp;
    class UnaryOp;
    class TernaryOp;
    class VarDefinition;
    class VarValue;
    class VarAssignment;
    class While;
    class Body;

    // @REVIEW: Don't free until end of compilation.
    // Also, nodes have to check if their properties are nullptr before freeing,
    // because a parse error might add a nullptr
    class Node {
        private:
            NodeType node_type;
            TokenPosition position;

        public:
            Node(NodeType type);
            Node(NodeType type, TokenPosition position);

            inline NodeType      get_type() const { return this->node_type; };
            inline TokenPosition get_position() const { return this->position; };

            /* Return pointers to nodes depending on the type you specify.
                The wrapper type MUST be the same as the node type. */
            Number* as_number();
            BinOp* as_bin_op();
            UnaryOp* as_unary_op();
            TernaryOp* as_ternary_op();
            VarDefinition* as_variable_definition();
            VarValue* as_variable_value();
            VarAssignment* as_variable_assignment();
            While* as_while_loop();
            Body* as_body();

            virtual ~Node() = default;
    };

    class Number : public Node {
        private:
            Values::number_t number;
        public:
            Number(Values::number_t number);

            inline Values::number_t get_number() const { return this->number; };
    };
    class True : public Node {
        public:
            True();
    };
    class False : public Node {
        public:
            False();
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

            ~BinOp();
    };
    class UnaryOp : public Node {
        private:
            Operations::UnaryOpType type;
            Node* argument;
        public:
            UnaryOp(Operations::UnaryOpType type, Node* argument);

            inline Operations::UnaryOpType get_type() const { return this->type; }
            inline Node* get_argument() const { return this->argument; }

            ~UnaryOp();
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

            ~TernaryOp();
    };

    /* E.g., var x = 3; */
    class VarDefinition : public Node {
        private:
            std::string* name;
            Node* value;
            Scopes::VariableType variable_type;
        public:
            VarDefinition(Scopes::VariableType variable_type, std::string* name, Node* value, TokenPosition pos);

            inline std::string*         get_name() const { return this->name; };
            inline Node*                get_value() const { return this->value; };
            inline Scopes::VariableType get_variable_type() const { return this->variable_type; };

            ~VarDefinition();
    };
    /* A reference to a variable, e.g. x */
    class VarValue : public Node {
        private:
            std::string* name;
        public:
            VarValue(std::string* name, TokenPosition pos);

            inline std::string* get_name() const { return this->name; };

            ~VarValue();
    };
    /* An assignment to a variable, e.g., x = 3 */
    class VarAssignment : public Node {
        private:
            Node* variable;
            Node* value;
        public:
            /* The position is the equals sign */
            VarAssignment(Node* variable, Node* value, TokenPosition position);

            inline Node* get_variable() const { return this->variable; };
            inline Node* get_value() const { return this->value; };

            ~VarAssignment();
    };

    class While : public Node {
        private:
            AST::Node* condition;
            AST::Node* block;
        public:
            While(AST::Node* condition, AST::Node* block);

            inline AST::Node* get_condition() const { return this->condition; };
            inline AST::Node* get_block() const { return this->block; };

            ~While();
    };

    class Body : public Node {
        private:
            std::vector<Node*> statements = std::vector<Node*>();
        public:
            Body();

            inline auto begin() const { return this->statements.begin(); };
            inline auto end() const { return this->statements.end(); };

            void add_statement(Node* statement);

            ~Body();
    };
}

#endif