#include "lexer.hpp"
#include "globals.hpp"

#ifdef DEBUG
    #include <cassert>
#endif

using namespace Scan;
using Position::TokenPosition;

const char* Scan::tok_type_to_string(TokType type) {
    switch (type) {
        case NUMBER: return "number";

        case PLUS: return "+";
        case MINUS: return "-";
        case STAR: return "*";
        case SLASH: return "/";
        case PERCENT: return "%";

        case EOI: return "end of file";

        default:
            /* We should not have to convert any other token to a string */
            #ifdef DEBUG
            assert(false);
            #endif
    }
};

Token::Token(TokType type, TokenPosition position) : type(type), position(position) {};
Token::Token(Value::number_t number, TokenPosition position) :
    type(TokType::NUMBER),
    payload(token_payload_t{ .num = number }),
    position(position) {};

Value::number_t Token::get_number() const {
    #ifdef DEBUG
    assert(this->type == TokType::NUMBER);
    #endif
    return this->payload.num;
}
#ifdef DEBUG
std::string Token::to_string() const {
    if (this->type == TokType::NUMBER) return std::to_string(this->get_number());
    else return "";
}
#endif

// Lexer helpers
namespace {
    inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

    bool is_hex_digit(char c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
    bool is_octal_digit(char c) { return c >= '0' && c <= '7'; }
};

Scanner::Scanner(std::string& str) : str(str) {};

char Scanner::current() const {
    if (this->ind >= this->str.length()) return '\0';
    return this->str.at(this->ind);
}
char Scanner::advance() {
    char curr = this->current();
    this->ind += 1;
    
    // Increase position variables
    if (curr == '\n') {
        this->line += 1;
        this->col = 0;
    }
    else {
        this->col += 1;
    }

    return curr;
}
char Scanner::peek(int skip_count) const {
    if (this->ind + skip_count >= this->str.length()) return '\0';
    return this->str.at(this->ind + skip_count);
}
bool Scanner::at_EOF() const {
    return this->ind >= this->str.length();
}

void Scanner::skip_whitespace() {
    char curr = this->current();

    while (
        curr == '\f' ||
        curr == '\n' ||
        curr == '\r' ||
        curr == '\f' ||
        curr == '\v' ||
        curr == ' '
    ) {
        this->advance();
        curr = this->current();
    }
}

TokenPosition Scanner::make_single_line_position(int length) const {
    return TokenPosition{ .line = this->line, .col = this->col - length, .length = length };

};

Token Scanner::next_token() {
    this->skip_whitespace();

    // If we're at the end of the file, just return an EOI
    if (this->current() == '\0') return Token(
        TokType::EOI,
        TokenPosition{ .line = this->line, col = this->col, .length = 0 }
    );

    char curr = this->current();

    // Constant symbols
    switch (curr) {
        case '+': this->advance(); return Token(TokType::PLUS, this->make_single_line_position(1));
        case '-': this->advance(); return Token(TokType::MINUS, this->make_single_line_position(1));
        case '*': this->advance(); return Token(TokType::STAR, this->make_single_line_position(1));
        case '/': this->advance(); return Token(TokType::SLASH, this->make_single_line_position(1));
        case '%': this->advance(); return Token(TokType::PERCENT, this->make_single_line_position(1));
    };

    // Number?
    if ( is_digit(curr) || (curr == '.' && is_digit(this->peek(1))) ) {
        bool dec = false;

        int length = 0;

        std::string number_string = "";

        while (!this->at_EOF()) {
            char curr = this->advance();
            if (is_digit(curr)) number_string += curr;
            else if (curr == '.') {
                // We can't have two decimal places in one number, so if we've
                // already found one, exit
                if (dec) break;
                number_string += '.';
                dec = true;
            }
            // If we didn't find a number or decimal place, break out of the loop
            else break;

            length += 1;
        }

        Value::number_t number;
        try {
            number = static_cast<Value::number_t>(std::stod(number_string));
        } catch (const std::exception&) {
            // If we catch an exception, that means the number failed to parse
            return Token(TokType::ERROR, Position::null_token_position);
        };

        return Token(number, this->make_single_line_position(length));
    }
}
