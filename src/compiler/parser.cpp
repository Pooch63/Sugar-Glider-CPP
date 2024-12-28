#include "parser.hpp"
#include "scopes.hpp"
#include "../globals.hpp"

#ifdef DEBUG
    #include <cassert>
#endif

using namespace Parse;
using Scan::TokType;
using Rules::nud_func_t, Rules::led_func_t;

/* Parse rule initialization > */
std::array<Rules::ParseRule, Scan::NUM_TOKEN_TYPES> Rules::rules = {};

// Rule functions >
AST::Node* Parse::Rules::number(Scan::Token& current, Parser* parser) {
    return new AST::Number(current.get_number());
}
AST::Node* Parse::Rules::keyword_constant(Scan::Token &current, Parser* parser) {
    switch (current.get_type()) {
        case TokType::TRUE: return new AST::True();
        case TokType::FALSE: return new AST::False();
        default:
            #ifdef DEBUG
            // We somehow got a token that was not a keyword constant
            assert(false);
            #endif
    }
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
    return new AST::UnaryOp(type, argument);
}
AST::Node* Parse::Rules::binary_op(Scan::Token& current, AST::Node* left, Parser* parser) {
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
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }

    AST::Node* right = parser->parse_precedence(static_cast<int>(prec) + 1);
    return new AST::BinOp(type, left, right);
}
AST::Node* Parse::Rules::paren_group(Scan::Token& current, Parser* parser) {
    AST::Node* expression = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(TokType::RPAREN);

    return expression;
};
AST::Node* Parse::Rules::ternary_op(Scan::Token& current, AST::Node* left, Parser* parser) {
    AST::Node* if_true = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(TokType::COLON);
    AST::Node* if_false = parser->parse_precedence((int)Precedence::PREC_NONE);

    return new AST::TernaryOp(left, if_true, if_false);
};

AST::Node* Parse::Rules::var_value(Scan::Token &current, Parser* parser) {
    // Make sure to stop the variable name from automatically being freed
    current.mark_payload();
    return new AST::VarValue(current.get_string(), current.get_position());
};
AST::Node* Parse::Rules::var_assignment(Scan::Token &current, AST::Node* left, Parser* parser) {
    if (left->get_type() != AST::NODE_VAR_VALUE) {
        parser->get_output().error(current.get_position(), "Only a variable may be followed by an equals sign. (=)");
    }

    AST::Node* value = parser->parse_expression();
    return new AST::VarAssignment(left, value, current.get_position());
};

bool Parser::rules_initialized = false;
void Parser::initialize_parse_rules() {
    using Rules::rules, Rules::ParseRule;
    rules[TokType::NUMBER] = ParseRule{
        .nud = Rules::number,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };
    rules[TokType::TRUE] =
        rules[TokType::FALSE] = ParseRule {
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

    rules[TokType::LPAREN] = ParseRule{
        .nud = Rules::paren_group,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
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
    #ifdef DEBUG
    /* You ABSOLUTELY CAN NOT initialize a parser without initializing the rules */
    assert(Parser::rules_initialized);
    #endif
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
    this->output.error(curr.get_position(), error_message);
    return false;
};
bool Parser::expect_symbol(TokType type, char* error_message) {
    Scan::Token curr = this->curr();
    if (curr.get_type() == type) {
        this->advance();
        return true;
    }

    this->output.error(curr.get_position(), error_message);
    return false;
};
bool Parser::expect(TokType type, char* error_message) {
    Scan::Token current = this->curr();
    this->advance();

    if (current.get_type() == type) {
        return true;
    }

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
            case TokType::EOI:
                return;
            default: break;
        }
        if (this->previous_token.get_type() == TokType::SEMICOLON) return;
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
AST::Node* Parser::parse_optionally_inlined_block() {
    if (this->curr().get_type() != TokType::LBRACKET) return this->parse_expression();

    // Go through {
    this->advance();

    AST::Body* body = new AST::Body();

    while (this->curr().get_type() != TokType::RBRACKET && !this->at_EOF()) {
        body->add_statement(this->parse_statement());
    }

    // Expect the }
    this->expect_symbol(TokType::RBRACKET);

    return body;
}

AST::Node* Parser::parse_precedence(int prec) {
    nud_func_t nud = this->get_nud();

    /* If there's no nud, it's not a prefix operator, so this token should not be here.
        Throw an error. */
    if (nud == nullptr) {
        char error_message[100];
        snprintf(error_message, 100, "Unexpected token type %s", Scan::tok_type_to_string(this->curr().get_type()));
        this->output.error(this->curr().get_position(), error_message);

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

AST::VarDefinition* Parser::parse_var_statement() {
    // Is it a const or a var?
    Scan::Token qualifier = this->curr();
    this->advance();

    bool got_identifier = this->expect_symbol(TokType::IDENTIFIER, "Expected identifier after var keyword");

    std::string* name = nullptr;
    TokenPosition position;

    /* Make sure to keep the identifier, so mark it if it exists. */
    if (got_identifier) {
        name = this->previous_token.get_string();
        position = this->previous_token.get_position();
        this->previous_token.mark_payload();
    }
    
    this->expect_symbol(TokType::EQUALS, "Expected equals sign after variable definition.");

    AST::Node* value = this->parse_expression();
    
    return got_identifier ? new AST::VarDefinition(
        qualifier.get_type() == TokType::VAR ? Intermediate::VariableType::MUTABLE : Intermediate::VariableType::CONSTANT,
        name,
        value,
        position) : nullptr;
}

AST::If* Parser::parse_if_statement() {
    // Go through if token
    this->advance();

    AST::Node* condition = this->parse_parenthesized_expression();
    AST::Node* block = this->parse_optionally_inlined_block();

    return new AST::If(condition, block);
}
AST::While* Parser::parse_while_statement() {
    // Go through while token
    this->advance();

    AST::Node* condition = this->parse_parenthesized_expression();
    AST::Node* block = this->parse_optionally_inlined_block();

    return new AST::While(condition, block);
}

AST::Break* Parser::parse_break_statement() {
    // Go through break token
    this->advance();
    Scan::Token break_= this->previous_token;

    return new AST::Break(break_.get_position());
}
AST::Continue* Parser::parse_continue_statement() {
    // Go through continue token
    this->advance();
    Scan::Token continue_= this->previous_token;

    return new AST::Continue(continue_.get_position());
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
        default:
            node = this->parse_expression();
            this->expect_symbol(TokType::SEMICOLON);
            break;
    }
    
    return node;
}

AST::Body* Parser::parse() {
    AST::Body* body = new AST::Body();

    while (!this->at_EOF()) {
        AST::Node* statement = this->parse_statement();
        if (statement != nullptr) body->add_statement(statement);
    }

    return body;
}