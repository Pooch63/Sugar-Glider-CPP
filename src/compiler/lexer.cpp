#include "lexer.hpp"
#include "../globals.hpp"

#include <unordered_map> // Keyword map

#ifdef DEBUG
    #include <cassert>
#endif

using namespace Scan;
using Position::TokenPosition;

const char* Scan::tok_type_to_string(TokType type) {
    switch (type) {
        case NUMBER: return "number";
        case STRING: return "string";

        case IDENTIFIER: return "identifier";

        case PLUS: return "+";
        case MINUS: return "-";
        case STAR: return "*";
        case SLASH: return "/";
        case PERCENT: return "%";

        case EQUALS: return "=";

        case LPAREN: return "(";
        case RPAREN: return ")";
        case LBRACKET: return "{";
        case RBRACKET: return "}";

        case QUESTION_MARK: return "?";
        case COLON: return ":";
        case SEMICOLON: return ";";

        case WHILE: return "while";
        case CONST: return "const";
        case VAR: return "var";
        case TRUE: return "true";
        case FALSE: return "false";

        case EOI: return "end of file";

        case START_DELIMETER: return "INTERNAL(start_delimeter)";

        default:
            /* We should not have to convert any other token to a string */
            #ifdef DEBUG
            assert(false);
            #endif
    }
};
std::string Scan::tok_to_concise_string(Token token) {
    switch (token.get_type()) {
        case NUMBER: return std::to_string(token.get_number());
        case IDENTIFIER: return *token.get_string();
        default: return std::string(tok_type_to_string(token.get_type()));
    }
};

Token::Token(TokType type, TokenPosition position) : type(type), position(position) {};
Token::Token(Values::number_t number, TokenPosition position) :
    type(TokType::NUMBER),
    position(position)
{
    this->payload.num = number;
};
Token::Token(TokType type, std::string* str_, TokenPosition position) :
    type(type),
    position(position)
{
    this->payload.string_ = str_;
};

// Token::Token(const Token& token) {
//     this->type = token.type;
//     this->position = token.position;
//     this->free_payload = token.free_payload;

//     if (token.has_string_payload()) {
//         this->payload.string_ = token.get_string();
//     }
//     else if(token.type == TokType::NUMBER) {
//         this->payload.num = token.get_number();
//     }
// };

void Token::free() {
    if (!this->free_payload) return;

    if (this->has_string_payload()) {
        printf("DEBUG freed string \"%s\" (%p)\n", this->get_string()->c_str(), this->get_string());
        delete this->get_string();
    }
}

Values::number_t Token::get_number() const {
    #ifdef DEBUG
    assert(this->type == TokType::NUMBER);
    #endif
    return this->payload.num;
}
std::string* Token::get_string() const {
    #ifdef DEBUG
    assert(this->has_string_payload());
    #endif
    return this->payload.string_;
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

    bool is_identifier_start(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    /* Can it go after the starting character in an identifier */
    bool is_identifier_middle(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    static std::unordered_map<std::string, TokType> keywords = {
        { "while", TokType::WHILE },
        { "const", TokType::CONST },
        { "var", TokType::VAR },
        { "true", TokType::TRUE },
        { "false", TokType::FALSE }
    };
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

    /* If we're at the end of the file, just return an EOI.
        Make the length 1 so that errors saying they expected another
        token at the end of the file have a carat. */
    if (this->current() == '\0') {
        return Token(
            TokType::EOI,
            TokenPosition{ .line = this->line, col = this->col, .length = 1 }
        );
    }

    char curr = this->current();

    // Constant symbols

    /* One-character symbols */
    TokType one_char_type = TokType::ERROR;
    switch (curr) {
        case '+': one_char_type = TokType::PLUS; break;
        case '-': one_char_type = TokType::MINUS; break;
        case '*': one_char_type = TokType::STAR; break;
        case '/': one_char_type = TokType::SLASH; break;
        case '%': one_char_type = TokType::PERCENT; break;

        case '=': one_char_type = TokType::EQUALS; break;

        case '(': one_char_type = TokType::LPAREN; break;
        case ')': one_char_type = TokType::RPAREN; break;
        case '{': one_char_type = TokType::LBRACKET; break;
        case '}': one_char_type = TokType::RBRACKET; break;

        case '?': one_char_type = TokType::QUESTION_MARK; break;
        case ':': one_char_type = TokType::COLON; break;
        case ';': one_char_type = TokType::SEMICOLON; break;
    };
    if (one_char_type != TokType::ERROR) {
        this->advance();
        return Token(one_char_type, this->make_single_line_position(1));
    }

    // Number?
    if ( is_digit(curr) || (curr == '.' && is_digit(this->peek(1))) ) {
        bool dec = false;

        int length = 0;

        std::string number_string = "";

        while (!this->at_EOF()) {
            char curr = this->current();
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
            // Only advance if it was a valid character.
            this->advance();
        }

        Values::number_t number;
        try {
            number = static_cast<Values::number_t>(std::stod(number_string));
        } catch (const std::exception&) {
            // If we catch an exception, that means the number failed to parse
            return Token(TokType::ERROR, Position::null_token_position);
        };

        return Token(number, this->make_single_line_position(length));
    }

    // Identifier or keyword
    if ( is_identifier_start(curr) ) {
        // Start the string with the identifier start
        std::string* identifier = new std::string(1, curr);

        int start_line = this->line,
            start_col = this->col,
            start_ind = this->ind;

        this->advance();

        while ( !this->at_EOF() && is_identifier_middle(this->current()) ) {
            identifier->append(1, this->advance());
        }

        TokenPosition position = TokenPosition{
                .line = start_line, .col = start_col,
                .length = static_cast<int>(this->ind) - start_ind };

        auto keyword = keywords.find(*identifier);
        if (keyword != keywords.end()) {
            /* We don't need the string object anymore */
            delete identifier;
            return Token(keyword->second, position);
        }

        /* If it wasn't a keyword, it must have been an identifier */
        Token tok = Token(TokType::IDENTIFIER, identifier, position);
        return tok;
    }

    /* If there was an unknown token, error and then  */
    std::cout << "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR" << std::endl;
}
