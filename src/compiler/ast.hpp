#ifndef _SGCPP_AST_HPP
#define _SGCPP_AST_HPP

#include "lexer.hpp"
#include "../operations.hpp"
#include "scopes.hpp"

namespace AST {
    enum NodeType {
        NODE_ARRAY,
        NODE_ARRAY_INDEX,
        NODE_STRING,
        NODE_NUMBER,
        NODE_NULL,
        NODE_TRUE,
        NODE_FALSE,

        NODE_BINOP,
        NODE_UNARYOP,
        NODE_TERNARY_OP,

        NODE_VAR_DEFINITION,
        NODE_VAR_VALUE,
        NODE_VAR_ASSIGNMENT,

        NODE_IF,
        NODE_WHILE,
        NODE_BREAK,
        NODE_CONTINUE,

        NODE_FUNCTION_CALL,
        NODE_FUNCTION_DEFINITION,
        NODE_RETURN,

        NODE_BODY
    };

    bool node_is_expression(NodeType type);
    bool node_may_be_function(NodeType type);
    bool node_may_be_array(NodeType type);

    const char* node_type_to_string(NodeType type);

    class String;
    class Array;
    class ArrayIndex;
    class Number;
    class True;
    class False;
    class BinOp;
    class UnaryOp;
    class TernaryOp;
    class VarDefinition;
    class VarValue;
    class VarAssignment;
    class If;
    class While;
    class Break;
    class Continue;
    class FunctionCall;
    class Function;
    class Return;
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
            String* as_string();
            Array* as_array();
            ArrayIndex* as_array_index();
            Number* as_number();
            BinOp* as_bin_op();
            UnaryOp* as_unary_op();
            TernaryOp* as_ternary_op();
            VarDefinition* as_variable_definition();
            VarValue* as_variable_value();
            VarAssignment* as_variable_assignment();
            If* as_if_statement();
            While* as_while_loop();
            Break* as_break_statement();
            Continue* as_continue_statement();
            FunctionCall* as_function_call();
            Function* as_function();
            Return* as_return_statement();
            Body* as_body();

            virtual ~Node() = default;
    };

    class Array : public Node {
        private:
            std::vector<Node*> values = std::vector<Node*>();
        public:
            Array();

            inline auto begin() const noexcept { return this->values.begin(); };
            inline auto   end() const noexcept { return this->values.end(); };

            void add_element(Node *element);
            inline size_t element_count() const { return this->values.size(); };

            ~Array();
    };
    /* Used for both array indexing and value updating, because
        compilation logic for both is very similar */
    class ArrayIndex : public Node {
        private:
            Node *array;
            Node *index;
            // Can be nullptr, if so, it is just a value get
            Node *value;
        public:
            ArrayIndex(Node *array, Node *index, Node *value);

            inline Node* get_array() const noexcept { return this->array; };
            inline Node* get_index() const noexcept { return this->index; };
            inline bool  is_value_get() const noexcept {
                return this->value == nullptr;
            }
            inline Node* get_value() const {
                #ifdef DEBUG
                assert(!this->is_value_get());
                #endif
                return this->value;
            };
            
            ~ArrayIndex();
    };
    class String : public Node {
        private:
            std::string* str;
        public:
            String(std::string* str, TokenPosition pos);

            inline std::string* get_string() const noexcept { return this->str; };

            ~String();
    };
    class Number : public Node {
        private:
            Values::number_t number;
        public:
            Number(Values::number_t number);

            inline Values::number_t get_number() const noexcept { return this->number; };
    };
    class Null : public Node {
        public:
            Null();
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
            /* Doesn't account for function variables or closures.
                Just stores whether it's mutable ro immutable. */
            Intermediate::VariableType basic_variable_type;
        public:
            VarDefinition(Intermediate::VariableType basic_variable_type, std::string* name, Node* value, TokenPosition pos);

            inline std::string*               get_name() const { return this->name; };
            inline Node*                      get_value() const { return this->value; };
            inline Intermediate::VariableType get_basic_variable_type() const { return this->basic_variable_type; };

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

    class If : public Node {
        private:
            AST::Node* condition;
            AST::Node* block;
        public:
            If(AST::Node* condition, AST::Node* block);

            inline AST::Node* get_condition() const { return this->condition; };
            inline AST::Node* get_block() const { return this->block; };

            ~If();
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
    class Break : public Node {
        public:
            Break(TokenPosition position);
    };
    class Continue : public Node {
        public:
            Continue(TokenPosition position);
    };

    class FunctionCall : public Node {
        private:
            Node* function;
            std::vector<Node*> arguments = std::vector<Node*>();
        public:
            FunctionCall(Node* function);

            inline auto          begin() const { return this->arguments.begin(); };
            inline auto            end() const { return this->arguments.end(); };
            inline auto argument_count() const { return this->arguments.size(); };

            inline Node* get_function() const { return this->function; };

            void add_argument(Node* argument);

            ~FunctionCall();
    };
    class Function : public Node {
        private:
            std::string* name;
            std::vector<std::string*> arguments = std::vector<std::string*>();
            Node* function_body;
        public:
            Function(std::string* name, TokenPosition name_position);

            inline auto            begin() const { return this->arguments.begin(); };
            inline auto              end() const { return this->arguments.end(); };
            inline std::string* get_name() const { return this->name; };
            inline Node*        get_body() const { return this->function_body; };

            void add_argument(std::string* argument);
            void set_body(Node* Body);

            ~Function();
    };
    class Return : public Node {
        private:
            Node *return_value;
        public:
            Return(Node *return_value, TokenPosition return_position);

            inline Node *get_return_value() const { return this->return_value; };

            ~Return();
    };

    class Body : public Node {
        private:
            /* Whether or not this node should create a scope during compilation.
                Default is yes. */
            bool should_create_scope = true;

            std::vector<Node*> statements = std::vector<Node*>();
        public:
            Body();

            inline auto begin() const { return this->statements.begin(); };
            inline auto end() const { return this->statements.end(); };

            void add_statement(Node* statement);
            // Stop the body from creating a scope. Used if the compiler creates
            // its own, special scope, and doesn't need another.
            void reject_scope();

            inline bool will_create_scope() const { return this->should_create_scope; };

            ~Body();
    };
}

#endif