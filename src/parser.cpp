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
    AST::Node* argument = parser->parse_precedence((int)Precedence::PREC_UNARY);
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

bool Parser::rules_initialized = false;
void Parser::initialize_parse_rules() {
    using Rules::rules, Rules::ParseRule;
    rules[Scan::TokType::NUMBER] = ParseRule{
        .nud = Rules::number,
        .led = nullptr,
        .precedence = Precedence::PREC_NONE
    };
    rules[Scan::TokType::PLUS] =
        rules[Scan::TokType::STAR] = rules[Scan::TokType::SLASH] =
        rules[Scan::TokType::PERCENT] =
        ParseRule{
            .nud = nullptr,
            .led = Rules::binary_op,
            .precedence = Precedence::PREC_NONE
        };
    rules[Scan::TokType::MINUS] = ParseRule{
        .nud = Rules::unary_op,
        .led = Rules::binary_op,
        .precedence = Precedence::PREC_TERM
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
};

Scan::Token Parser::advance() {
    Scan::Token current = this->curr();
    this->current_token = this->scanner.next_token();

    return current;
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