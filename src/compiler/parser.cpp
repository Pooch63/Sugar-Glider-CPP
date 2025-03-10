#include "parser.hpp"
#include "scopes.hpp"
#include "../globals.hpp"
#include "../memory.hpp"

#ifdef DEBUG
    #include <cassert>
#endif

using namespace Parse;
using Scan::TokType;
using Rules::nud_func_t, Rules::led_func_t;

/* Parse rule initialization > */
std::array<Rules::ParseRule, Scan::NUM_TOKEN_TYPES> Rules::rules = {};

// Rule functions >
AST::Node* Parse::Rules::number(Scan::Token& current, [[maybe_unused]] Parser* parser) {
    return Allocate<AST::Number>::create(current.get_number());
}
AST::Node* Parse::Rules::parse_string(Scan::Token &current, Parser* parser) {
    // You can do something like "a""b" to concatenate into "ab", so allow for that
    std::string* str = current.get_string();
    current.mark_payload();

    while (parser->curr().get_type() == TokType::STRING) {
        std::string* next = parser->curr().get_string();
        str->append(std::string(*next));
        parser->advance();
    }

    return Allocate<AST::String>::create(str, current.get_position());
}
AST::Node* Parse::Rules::keyword_constant(Scan::Token &current, [[maybe_unused]] Parser* parser) {
    switch (current.get_type()) {
        case TokType::TRUE: return Allocate<AST::True>::create();
        case TokType::FALSE: return Allocate<AST::False>::create();
        case TokType::NULL_TOKEN: return Allocate<AST::Null>::create();
        default:
            throw sg_assert_error("Parser tried to parse non-literal token as literal");
    }
}
AST::Node* Parse::Rules::parse_array([[maybe_unused]] Scan::Token &current, Parser* parser) {
    AST::Array *array = Allocate<AST::Array>::create();

    bool found_element = false;

    while (!parser->at_EOF() && parser->curr().get_type() != TokType::RBRACE) {
        // NEED comma before element
        if (found_element) {
            parser->expect_symbol(
                TokType::COMMA, "Comma expected before additional array element");
        }

        AST::Node *element = parser->parse_expression();
        array->add_element(element);
        found_element = true;
    }
    parser->expect_symbol(TokType::RBRACE, "Array was not closed with a ']'");

    return array;
}
AST::Node *Parse::Rules::parse_array_index([[maybe_unused]] Scan::Token &current, AST::Node *left, Parser *parser) {
    if (!AST::node_may_be_array(left->get_type())) {
        char error_message[100];
        snprintf(error_message, 100, "Cannot index non-array type %s", AST::node_type_to_string(left->get_type()));
        parser->get_output().error(current.get_position(), "", Errors::PARSE_ERROR);
    }

    AST::Node *index = parser->parse_expression();
    parser->expect_symbol(TokType::RBRACE, "Expected ']' to close off array index");

    AST::Node *value = nullptr;

    if (parser->curr().get_type() == TokType::EQUALS) {
        parser->advance();
        value = parser->parse_expression();
    }

    return Allocate<AST::ArrayIndex>::create(left, index, value);
}

AST::Node* Parse::Rules::unary_op(Scan::Token& current, Parser* parser) {
    Operations::UnaryOpType type;
    switch (current.get_type()) {
        case TokType::MINUS:
            type = Operations::UNARY_NEGATE;
            break;
        default:
            #ifdef DEBUG
            assert(false);
            #endif
            break;
    }
    AST::Node* argument = parser->parse_precedence((int)Precedence::PREC_UNARY + 1);
    return Allocate<AST::UnaryOp>::create(type, argument);
}
AST::Node* Parse::Rules::binary_op(Scan::Token &current, AST::Node* left, Parser* parser) {
    Operations::BinOpType type;
    Precedence prec;

    switch (current.get_type()) {
        case TokType::PLUS:
            type = Operations::BINOP_ADD;
            prec = Precedence::PREC_TERM;
            break;
        case TokType::MINUS:
            type = Operations::BINOP_SUB;
            prec = Precedence::PREC_TERM;
            break;
        case TokType::STAR:
            type = Operations::BINOP_MUL;
            prec = Precedence::PREC_FACTOR;
            break;
        case TokType::SLASH:
            type = Operations::BINOP_DIV;
            prec = Precedence::PREC_FACTOR;
            break;
        case TokType::PERCENT:
            type = Operations::BINOP_MOD;
            prec = Precedence::PREC_FACTOR;
            break;
        case TokType::LESS_THAN:
            type = Operations::BINOP_LESS_THAN;
            prec = Precedence::PREC_RELATIONAL;
            break;
        case TokType::GREATER_THAN:
            type = Operations::BINOP_GREATER_THAN;
            prec = Precedence::PREC_RELATIONAL;
            break;
        case TokType::LESS_THAN_EQUAL:
            type = Operations::BINOP_LESS_THAN_OR_EQUAL;
            prec = Precedence::PREC_RELATIONAL;
            break;
        case TokType::GREATER_THAN_EQUAL:
            type = Operations::BINOP_GREATER_THAN_OR_EQUAL;
            prec = Precedence::PREC_RELATIONAL;
            break;
        case TokType::BANG_EQ:
            type = Operations::BINOP_NOT_EQUAL_TO;
            prec = Precedence::PREC_EQUALITY;
            break;
        case TokType::EQUALS_EQUALS:
            type = Operations::BINOP_EQUAL_TO;
            prec = Precedence::PREC_EQUALITY;
            break;
        default:
            throw sg_assert_error("Parser tried to parse unknown token as binary operator");
    }

    AST::Node* right = parser->parse_precedence(static_cast<int>(prec) + 1);
    return Allocate<AST::BinOp>::create(type, left, right);
}
AST::Node* Parse::Rules::paren_group([[maybe_unused]] Scan::Token &current, Parser* parser) {
    AST::Node* expression = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(TokType::RPAREN);

    return expression;
};
AST::Node* Parse::Rules::ternary_op([[maybe_unused]] Scan::Token &current, AST::Node* left, Parser* parser) {
    AST::Node* if_true = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(TokType::COLON, "Expected colon after '?' in ternary expression");
    AST::Node* if_false = parser->parse_precedence((int)Precedence::PREC_NONE);

    return Allocate<AST::TernaryOp>::create(left, if_true, if_false);
};

AST::Node* Parse::Rules::function_call(Scan::Token &current, AST::Node* left, Parser* parser) {
    if (!AST::node_may_be_function(left->get_type())) {
        char error_message[100];
        snprintf(error_message, 100, "Cannot call non-function type %s", AST::node_type_to_string(left->get_type()));
        parser->get_output().error(
            current.get_position(),
            error_message,
            Errors::PARSE_ERROR
        );
    }

    AST::FunctionCall* call = Allocate<AST::FunctionCall>::create(left);

    int arg_count = 0;
    if (parser->curr().get_type() != TokType::RPAREN) {
        while (parser->curr().get_type() != TokType::RPAREN) {
            // Warn if we've already seen the max number of arguments
            if (arg_count == MAX_FUNCTION_ARGUMENTS) {
                // Use the macro to avoid an unncessary snprintf format
                #define CREATE_MAX_ARG_ERROR(x) "A function call may only pass up to " STRINGIFY(x) " arguments"
                parser->get_output().error(parser->curr().get_position(), CREATE_MAX_ARG_ERROR(MAX_FUNCTION_ARGUMENTS), Errors::PARSE_ERROR);
                #undef CREATE_MAX_ARG_ERROR
            }

            AST::Node* argument = parser->parse_expression();
            
            if (argument == nullptr) break;
            call->add_argument(argument);

            arg_count += 1;

            if (parser->curr().get_type() == TokType::COMMA) {
                parser->advance();
            }
            else {
                break;
            }
        }
    }

    parser->expect_symbol(TokType::RPAREN, "Expected ')' after function call");

    return call;
}
AST::Node *Parse::Rules::parse_dot_get([[maybe_unused]] Scan::Token &current, AST::Node *left, Parser *parser) {
    if (!AST::node_may_be_object(left->get_type())) {
        char error_message[100];
        snprintf(error_message, 100, "Cannot use dot operator (.) on non-object %s", AST::node_type_to_string(left->get_type()));
        parser->get_output().error(
            current.get_position(),
            error_message,
            Errors::PARSE_ERROR
        );
    }
    
    bool next_is_identifier = parser->expect(TokType::IDENTIFIER, "Expected identifier to complete dot qualifier");
    
    std::string *right = next_is_identifier ? parser->previous().get_string() : nullptr;
    if (next_is_identifier) parser->previous().mark_payload();
    return Allocate<AST::Dot>::create(left, right);
};


AST::Node* Parse::Rules::var_value(Scan::Token &current, [[maybe_unused]] Parser* parser) {
    // Make sure to stop the variable name from automatically being freed
    current.mark_payload();
    return Allocate<AST::VarValue>::create(current.get_string(), current.get_position());
};
AST::Node* Parse::Rules::var_assignment(Scan::Token &current, AST::Node* left, Parser* parser) {
    if (left->get_type() != AST::NODE_VAR_VALUE) {
        parser->get_output().error(current.get_position(), "Only a variable may be followed by an equals sign. (=)", Errors::PARSE_ERROR);
    }

    AST::Node* value = parser->parse_expression();
    return Allocate<AST::VarAssignment>::create(left, value, current.get_position());
};

bool Parser::rules_initialized = false;
void Parser::initialize_parse_rules() {
    using Rules::rules, Rules::ParseRule;
    rules[TokType::STRING] = ParseRule{
        .nud = Rules::parse_string,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };
    rules[TokType::NUMBER] = ParseRule{
        .nud = Rules::number,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };
    rules[TokType::TRUE] =
        rules[TokType::FALSE] = rules[TokType::NULL_TOKEN] =
        ParseRule {
            .nud = Rules::keyword_constant,
            .led = nullptr,
            .precedence = Precedence::PREC_NONE
        };
    rules[TokType::IDENTIFIER] = ParseRule{
        .nud = Rules::var_value,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };

    rules[TokType::PLUS] = ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_TERM
        };
    rules[TokType::MINUS] = ParseRule{
        .nud = Rules::unary_op,
        .led = Rules::binary_op,
        .precedence = Precedence::PREC_TERM
    };
    rules[TokType::STAR] =
        rules[TokType::SLASH] = rules[TokType::PERCENT] =
        ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_FACTOR
        };
    rules[TokType::LESS_THAN] = rules[TokType::GREATER_THAN] =
        rules[TokType::LESS_THAN_EQUAL] = rules[TokType::GREATER_THAN_EQUAL] =
        ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_RELATIONAL
        };
    rules[TokType::BANG_EQ] = rules[TokType::EQUALS_EQUALS] = ParseRule{
        .nud = nullptr,
        .led = Rules::binary_op,
        .precedence = Precedence::PREC_EQUALITY
    };
    rules[TokType::LPAREN] = ParseRule{
        .nud = Rules::paren_group,
        .led = Rules::function_call,
        .precedence = Precedence::PREC_CALL
    };
    rules[TokType::QUESTION_MARK] = ParseRule{
        .nud = nullptr,
        .led = Rules::ternary_op,
        .precedence = Precedence::PREC_ASSIGNMENT_OR_TERNARY
    };
    rules[TokType::EQUALS] = ParseRule{
        .nud = nullptr,
        .led = Rules::var_assignment,
        .precedence = Precedence::PREC_ASSIGNMENT_OR_TERNARY
    };

    rules[TokType::LBRACE] = ParseRule{
        .nud = Rules::parse_array,
        .led = Rules::parse_array_index,
        .precedence = Precedence::PREC_NONE
    };
    rules[TokType::DOT] = ParseRule{
        .nud = nullptr,
        .led = Rules::parse_dot_get,
        .precedence = Precedence::PREC_DOT
    };

    Parser::rules_initialized = true;
}
// < Rule functions
/* < Parse rule initialization */

Parser::Parser(Scan::Scanner& scanner, Output& output) :
    scanner(scanner),
    output(output),
    previous_token(Scan::start),
    current_token(scanner.next_real_token())
{
    if (!Parser::rules_initialized) Parser::initialize_parse_rules();
}

void Parser::advance() {this->previous_token.free();
    this->previous_token = this->current_token;
    this->current_token = this->scanner.next_real_token();
}
bool Parser::at_EOF() const {
    return this->current_token.get_type() == TokType::EOI;
}

bool Parser::expect_symbol(TokType type) {
    Scan::Token curr = this->curr();
    if (curr.get_type() == type) {
        this->advance();
        return true;
    }

    char error_message[100];
    snprintf(
        error_message, 100,
        "Unexpected token %s (%s) -- expected token of type '%s'",
        Scan::tok_type_to_string(this->curr().get_type()),
        Scan::tok_to_concise_string(this->curr()).c_str(),
        Scan::tok_type_to_string(type));
    this->output.error(curr.get_position(), error_message, Errors::PARSE_ERROR);
    return false;
};
bool Parser::expect_symbol(TokType type, const char* error_message) {
    Scan::Token curr = this->curr();
    if (curr.get_type() == type) {
        this->advance();
        return true;
    }

    this->output.error(curr.get_position(), error_message, Errors::PARSE_ERROR);
    return false;
};
bool Parser::expect(TokType type, char* error_message) {
    /* Intentional choice: I felt that const_cast was appropriate here to avoid the EXACT SAME CODE
        implemented twice. */
    return this->expect(type, const_cast<const char*>(error_message));
}
bool Parser::expect(TokType type, const char* error_message) {
    Scan::Token current = this->curr();
    this->advance();

    if (current.get_type() == type) {
        return true;
    }

    this->output.error(current.get_position(), error_message, Errors::PARSE_ERROR);

    return false;
}

void Parser::skip_semicolons() {
    while (this->curr().get_type() == TokType::SEMICOLON) {
        this->advance();
    }
}
void Parser::synchronize() {
    using namespace Scan;
    while (true) {
        switch (this->curr().get_type()) {
            case TokType::CONST:
            case TokType::VAR:
            case TokType::WHILE:
            case TokType::IF:
            case TokType::SEMICOLON:
            case TokType::EOI:
                return;
            default: break;
        }
        this->advance();
    }
}

Rules::ParseRule Parser::get_parse_rule() {
    return Rules::rules.at(static_cast<int>(this->curr().get_type()));
}
nud_func_t Parser::get_nud() {
    return this->get_parse_rule().nud;
}
led_func_t Parser::get_led() {
    return this->get_parse_rule().led;
}

AST::Node* Parser::parse_parenthesized_expression() {
    this->expect_symbol(TokType::LPAREN);
    AST::Node* expression = this->parse_expression();
    this->expect_symbol(TokType::RPAREN);

    return expression;
};
AST::Body* Parser::parse_braced_block() {
    // Go through {
    this->advance();

    AST::Body* body = Allocate<AST::Body>::create();

    while (this->curr().get_type() != TokType::RBRACKET && !this->at_EOF()) {
        body->add_statement(this->parse_statement());
    }

    // Expect the }
    this->expect_symbol(TokType::RBRACKET);

    return body;
}
AST::Node* Parser::parse_optionally_inlined_block() {
    if (this->curr().get_type() != TokType::LBRACKET) return this->parse_expression();

    return this->parse_braced_block();
}

AST::Node* Parser::parse_precedence(int prec) {
    nud_func_t nud = this->get_nud();

    /* If there's no nud, it's not a prefix operator, so this token should not be here.
        Throw an error. */
    if (nud == nullptr) {
        char error_message[100];
        snprintf(error_message, 100, "Unexpected token type %s", Scan::tok_type_to_string(this->curr().get_type()));
        this->output.error(this->curr().get_position(), error_message, Errors::PARSE_ERROR);

        // Enter panic mode
        this->synchronize();
        
        return nullptr;
    }

    /* Save the start because we have to advance before calling nud. */
    this->advance();
    AST::Node* left = nud(this->previous_token, this);

    while (this->get_parse_rule().precedence >= prec) {
        led_func_t led = this->get_led();
        /* If there is no led, it's not an infix operator, so stop */
        if (led == nullptr) break;

        /* Make sure to consume the next token before calling the led.
            First, though, save the led token */
        Scan::Token infix = this->curr();
        this->advance();

        left = led(infix, left, this);
    }

    return left;
}

AST::Node* Parser::parse_expression() {
    return this->parse_precedence(Precedence::PREC_NONE);
}

AST::VarDefinition *Parser::parse_var_def_no_header(Intermediate::VariableType type) {
    bool got_identifier = this->expect_symbol(TokType::IDENTIFIER, "Expected identifier after var keyword");

    std::string *name = nullptr;
    TokenPosition position;

    /* Make sure to keep the identifier, so mark it if it exists. */
    if (got_identifier) {
        name = this->previous_token.get_string();
        position = this->previous_token.get_position();
        this->previous_token.mark_payload();
    }

    this->expect_symbol(TokType::EQUALS, "Expected equals sign after variable definition.");
    AST::Node* value = this->parse_expression();

    return got_identifier ? Allocate<AST::VarDefinition>::create(
        type,
        name,
        value,
        position) : nullptr;
}
AST::VarDefinition *Parser::parse_var_statement() {
    // Is it a const or a var?
    Scan::Token qualifier = this->curr();
    this->advance();

    Intermediate::VariableType type =
        qualifier.get_type() == TokType::VAR ? Intermediate::VariableType::GLOBAL_MUTABLE : Intermediate::VariableType::GLOBAL_CONSTANT;

    AST::VarDefinition *def = this->parse_var_def_no_header(type);
    
    return def;
}

AST::If* Parser::parse_if_statement() {
    // Go through if token
    this->advance();

    AST::Node* condition = this->parse_parenthesized_expression();
    AST::Node* block = this->parse_optionally_inlined_block();

    return Allocate<AST::If>::create(condition, block);
}
AST::While* Parser::parse_while_statement() {
    // Go through while token
    this->advance();

    AST::Node* condition = this->parse_parenthesized_expression();
    AST::Node* block = this->parse_optionally_inlined_block();

    return Allocate<AST::While>::create(condition, block);
}

AST::Break* Parser::parse_break_statement() {
    // Go through break token
    this->advance();
    Scan::Token break_= this->previous_token;

    return Allocate<AST::Break>::create(break_.get_position());
}
AST::Continue* Parser::parse_continue_statement() {
    // Go through continue token
    this->advance();
    Scan::Token continue_= this->previous_token;

    return Allocate<AST::Continue>::create(continue_.get_position());
}
AST::Return* Parser::parse_return_statement() {
    // Go through return token
    this->advance();

    TokenPosition return_position = this->previous_token.get_position();
    AST::Node* value = this->parse_expression();

    this->expect_symbol(TokType::SEMICOLON, "Semicolon expected after return statement");
    return Allocate<AST::Return>::create(value, return_position);
};

std::string* Parser::parse_function_parameter() {
    bool found_identifier = this->expect(TokType::IDENTIFIER, "Expected identifier as function parameter");
    if (found_identifier) this->previous_token.mark_payload();
    return found_identifier ? this->previous_token.get_string() : nullptr;
}
AST::Function* Parser::parse_function() {
    // Go through function token
    this->advance();

    bool found_identifier = this->expect(TokType::IDENTIFIER, "Expected identifier to describe function");
    std::string* name = found_identifier ? this->previous_token.get_string() : nullptr;
    if (found_identifier) this->previous_token.mark_payload();

    AST::Function* function = Allocate<AST::Function>::create(name, this->previous_token.get_position());

    this->expect_symbol(TokType::LPAREN);

    if (this->current_token.get_type() != TokType::RPAREN) {
        while (this->curr().get_type() != TokType::RPAREN && this->curr().get_type() != TokType::EOI) {
            std::string* argument = this->parse_function_parameter();
            
            if (argument == nullptr) break;
            function->add_argument(argument);

            if (this->curr().get_type() == TokType::COMMA) {
                this->advance();
            }
            else {
                break;
            }
        }
    }

    this->expect_symbol(TokType::RPAREN, "Expected ) after function arguments");

    function->set_body(this->parse_braced_block());
    return function;
}

AST::Node* Parser::parse_statement() {
    // Skip over redundant semicolons
    this->skip_semicolons();

    AST::Node* node;
    switch (this->curr().get_type()) {
        case TokType::EOI:
            node = nullptr;
            break;
        case TokType::CONST:
        case TokType::VAR:
            node = this->parse_var_statement();
            this->expect_symbol(TokType::SEMICOLON);
            break;
        case TokType::WHILE:
            node = this->parse_while_statement();
            break;
        case TokType::BREAK:
            node = this->parse_break_statement();
            this->expect_symbol(TokType::SEMICOLON);
            break;
        case TokType::CONTINUE:
            node = this->parse_continue_statement();
            this->expect_symbol(TokType::SEMICOLON);
            break;
        case TokType::IF:
            node = this->parse_if_statement();
            break;
        case TokType::FUNCTION:
            node = this->parse_function();
            break;
        case TokType::RETURN:
            node = this->parse_return_statement();
            break;
        default:
            node = this->parse_expression();
            this->expect_symbol(TokType::SEMICOLON);
            break;
    }
    
    return node;
}

AST::Body* Parser::parse() {
    AST::Body* body = Allocate<AST::Body>::create();

    while (!this->at_EOF()) {
        AST::Node* statement = this->parse_statement();
        if (statement != nullptr) body->add_statement(statement);
    }

    return body;
}