#ifndef _SGCPP_PARSER_HPP
#define _SGCPP_PARSER_HPP

#include "ast.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "operations.hpp"

#include <array>

namespace Parse {
    /* The precedence for each rule in our Pratt top-down parser.
        Because enums start at 0, the lowest-precedence rules must be specified first. */
    enum Precedence {
        // Used for unary operators that are NOT infix operators, e.g. numbers
        PREC_NONE,

        PREC_ASSIGNMENT_OR_TERNARY, // a = b OR a ? b : c

        // + or -
        PREC_TERM,
        // *, / or %
        PREC_FACTOR,

        PREC_UNARY
    };

    class Parser;

    namespace Rules {
        /* The parser will advance to the next token before calling the led or nud function,
            so these functions do not have to and SHOULD NOT.
            E.g., a number nud function does not have to advance past the number token. */
        typedef AST::Node *(*nud_func_t)(Scan::Token &current, Parser* parser);
        typedef AST::Node *(*led_func_t)(Scan::Token &current, AST::Node* left, Parser* parser);

        struct ParseRule {
            /* These are the functions used as a parse rule in a Pratt top-down parser.
                Note that many rules are ONLY prefix operators or ONLY infix operators,
                and thus, only need a nud function or a led function. This means that
                the nud function OR the led function may be a null reference. */
            nud_func_t nud = nullptr;
            led_func_t led = nullptr;
            Precedence precedence;
        };

        /* Parse rule table */
        extern std::array<ParseRule, Scan::NUM_TOKEN_TYPES> rules;

        /* Rule functions */
        AST::Node* number(Scan::Token &current, Parser* parser);
        // E.g., true, false, and null
        AST::Node* keyword_constant(Scan::Token &current, Parser* parser);

        AST::Node* unary_op(Scan::Token &current, Parser* parser);
        AST::Node* binary_op(Scan::Token &current, AST::Node* left, Parser* parser);
        AST::Node* ternary_op(Scan::Token &current, AST::Node* left, Parser* parser);
        AST::Node* paren_group(Scan::Token &current, Parser* parser);

        AST::Node* var_value(Scan::Token &current, Parser* parser);
        // Something like x = 3
        AST::Node* var_assignment(Scan::Token &current, AST::Node* left, Parser* parser);
    };

    class Parser {
        public:
            /* Static functions to initialize the parser rules.
                These only need to be called once, but there is a guard
                in case they are called more than that in order to prevent
                double initialization. */
            static bool rules_initialized;
            static void initialize_parse_rules();

        private:
            /* Whether or not we're on our first token. If we are,  */

            /* The scanner we'll get our tokens from */
            Scan::Scanner& scanner;
            /* The output class we use to error and warn. */
            Output& output;

            Scan::Token previous_token;
            /* Will be set to the first token of the stream */
            Scan::Token current_token;

            // Token helpers
            /* Current token */
            inline Scan::Token& curr() { return this->current_token; };
            /* Previous token */
            inline Scan::Token& previous() { return this->previous_token; };
            /* Continues forward with one token. */
            void advance();
            /* Returns whether or not there are only EOF tokens in the stream */
            bool at_EOF() const;

            // Rule helpers
            /* Returns the rule of the current token. */
            Rules::ParseRule get_parse_rule();
            /* Returns the nud function of the current token. (Could be null) */
            Rules::nud_func_t get_nud();
            /* Returns the led function of the current token. (Could be null) */
            Rules::led_func_t get_led();
        
            /* Panic until we find a syncronizing token */
            void synchronize();

            void skip_semicolons();

            /* Parses an expression within parentheses (()) */
            AST::Node* parse_parenthesized_expression();
            /* Parse either a block (code surrounded by {}), OR if that's not possible,
                parse a single statement. */
            AST::Node* parse_optionally_inlined_block();
        public:
            Parser(Scan::Scanner& scanner, Output& output);

            /* Pratt top-down parse expressions with this precedence or higher. */
            AST::Node* parse_precedence(int prec);

            /* Parsing functions. These do not consume the semicolon, even if it is necessary.
                That is the job of the caller. */
            AST::Node* parse_expression();
            AST::VarDefinition* parse_var_statement();
            AST::While* parse_while_statement();

            AST::Node* parse_statement();

            AST::Body* parse();

            // Error helpers (public so that the parsing functions can access them)
            // None of the expect functions enter panic mode.
            /* If the current token is the given type, advance and return true.
                Otherwise, DO NOT advance, error, and return false.
                Generates an error based on the token types.
                Meant to be used in situations where it's not absolutely necessary
                for the token to be there. */
            bool expect_symbol(Scan::TokType type);
            /* Same as expect_symbol with just the type, but a custom error message is provided. */
            bool expect_symbol(Scan::TokType type, char* error_message);
            /* If the current token is the given type, return true.
                Otherwise, error and return false.
                Advances no matter what. */
            bool expect(Scan::TokType type, char* error_message);

            inline Output &get_output() const { return this->output; };
    };
};

#endif