#include "globals.hpp"
#include "parser.hpp"

#ifdef DEBUG
    #include <cassert>
#endif

using namespace Parse;
using Rules::nud_func_t, Rules::led_func_t;

/* Parse rule initialization > */
std::array<Rules::ParseRule, Scan::NUM_TOKEN_TYPES> Rules::rules = {};

// Rule functions >
AST::Node* Parse::Rules::number(Scan::Token current, Parser* parser) {
    return new AST::Number(current.get_number());
}
AST::Node* Parse::Rules::unary_op(Scan::Token current, Parser* parser) {
    Operations::UnaryOpType type;
    switch (current.get_type()) {
        case Scan::TokType::MINUS:
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
AST::Node* Parse::Rules::binary_op(Scan::Token current, AST::Node* left, Parser* parser) {
    Operations::BinOpType type;
    Precedence prec;

    switch (current.get_type()) {
        case Scan::TokType::PLUS:
            type = Operations::BINOP_ADD;
            prec = Precedence::PREC_TERM;
            break;
        case Scan::TokType::MINUS:
            type = Operations::BINOP_SUB;
            prec = Precedence::PREC_TERM;
            break;
        case Scan::TokType::STAR:
            type = Operations::BINOP_MUL;
            prec = Precedence::PREC_FACTOR;
            break;
        case Scan::TokType::SLASH:
            type = Operations::BINOP_DIV;
            prec = Precedence::PREC_FACTOR;
            break;
        case Scan::TokType::PERCENT:
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
AST::Node* Parse::Rules::paren_group(Scan::Token current, Parser* parser) {
    AST::Node* expression = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(Scan::TokType::RPAREN);

    return expression;
};
AST::Node* Parse::Rules::ternary_op(Scan::Token current, AST::Node* left, Parser* parser) {
    AST::Node* if_true = parser->parse_precedence((int)Precedence::PREC_NONE);
    parser->expect_symbol(Scan::TokType::COLON);
    AST::Node* if_false = parser->parse_precedence((int)Precedence::PREC_NONE);

    return new AST::TernaryOp(left, if_true, if_false);
};

bool Parser::rules_initialized = false;
void Parser::initialize_parse_rules() {
    using Rules::rules, Rules::ParseRule;
    rules[Scan::TokType::NUMBER] = ParseRule{
        .nud = Rules::number,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };
    rules[Scan::TokType::PLUS] = ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_TERM
        };
    rules[Scan::TokType::MINUS] = ParseRule{
        .nud = Rules::unary_op,
        .led = Rules::binary_op,
        .precedence = Precedence::PREC_TERM
    };

    rules[Scan::TokType::STAR] =
        rules[Scan::TokType::SLASH] = rules[Scan::TokType::PERCENT] =
        ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_FACTOR
        };

    rules[Scan::TokType::LPAREN] = ParseRule{
        .nud = Rules::paren_group,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };

    rules[Scan::TokType::QUESTION_MARK] = ParseRule{
        .nud = nullptr,
        .led = Rules::ternary_op,
        .precedence = Precedence::PREC_ASSIGNMENT_OR_TERNARY
    };

    Parser::rules_initialized = true;
}
// < Rule functions
/* < Parse rule initialization */

Parser::Parser(Scan::Scanner& scanner, Output& output) :
    scanner(scanner), output(output), current_token(scanner.next_token())
{
    #ifdef DEBUG
    /* You ABSOLUTELY CAN NOT initialize a parser without initializing the rules */
    assert(Parser::rules_initialized);
    #endif
}

void Parser::advance() {
    this->current_token = this->scanner.next_token();
}

Scan::Token* Parser::expect_symbol(Scan::TokType type) {
    Scan::Token curr = this->curr();
    if (curr.get_type() == type) {
        this->advance();
        return &this->current_token;
    }

    char error_message[100];
    snprintf(
        error_message, 100,
        "Unexpected token %s (%s) -- expected token of type '%s'",
        Scan::tok_type_to_string(this->curr().get_type()),
        Scan::tok_to_concise_string(this->curr()).c_str(),
        Scan::tok_type_to_string(type));
    this->output.error(curr.get_position(), error_message);
    return nullptr;
};
Scan::Token* Parser::expect(Scan::TokType type, char* error_message) {
    Scan::Token current = this->curr();
    this->advance();

    if (current.get_type() == type) {
        return &this->current_token;
    }

    return nullptr;
}

void Parser::synchronize() {
    using namespace Scan;
    while (true) {
        switch (this->curr().get_type()) {
            case TokType::EOI:
                return;
            default: break;
        }
        this->advance();
    }
}

Rules::ParseRule Parser::get_parse_rule() const {
    return Rules::rules.at(static_cast<int>(this->curr().get_type()));
}
nud_func_t Parser::get_nud() const {
    return this->get_parse_rule().nud;
}
led_func_t Parser::get_led() const {
    return this->get_parse_rule().led;
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
    Scan::Token start = this->curr();
    this->advance();

    AST::Node* left = nud(start, this);

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