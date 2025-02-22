#include "lexer.hpp"
#include "../globals.hpp"
#include "../utils.hpp"

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
        case EQUALS_EQUALS: return "==";
        case BANG_EQ: return "!=";

        case LPAREN: return "(";
        case RPAREN: return ")";
        case LBRACE: return "[";
        case RBRACE: return "]";
        case LBRACKET: return "{";
        case RBRACKET: return "}";
        case LESS_THAN: return "<";
        case GREATER_THAN: return ">";
        case LESS_THAN_EQUAL: return "<=";
        case GREATER_THAN_EQUAL: return ">=";

        case QUESTION_MARK: return "?";
        case COLON: return ":";
        case SEMICOLON: return ";";
        case COMMA: return ",";
        case DOT: return ".";

        case BREAK: return "break";
        case CONST: return "const";
        case CONTINUE: return "continue";
        case FALSE: return "false";
        case FUNCTION: return "function";
        case IF: return "if";
        case NULL_TOKEN: return "null";
        case RETURN: return "return";
        case TRUE: return "true";
        case VAR: return "var";
        case WHILE: return "while";

        case EOI: return "end of file";

        case START_DELIMETER: return "INTERNAL(start_delimeter)";

        default:
            /* We should not have to convert any other token to a string */
            throw sg_assert_error("Unknown token type to convert to string");
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

void Token::free() {
    if (!this->free_payload) return;

    if (this->has_string_payload()) {
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
    bool is_octal_digit(char c) { return c >= '0' && c <= '7'; }

    bool is_identifier_start(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    /* Can it go after the starting character in an identifier */
    bool is_identifier_middle(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    std::unordered_map<std::string, TokType> keywords = {
        { "break", TokType::BREAK },
        { "const", TokType::CONST },
        { "continue", TokType::CONTINUE },
        { "false", TokType::FALSE },
        { "function", TokType::FUNCTION },
        { "if", TokType::IF },
        { "null", TokType::NULL_TOKEN },
        { "return", TokType::RETURN },
        { "true", TokType::TRUE },
        { "var", TokType::VAR },
        { "while", TokType::WHILE }
    };

    enum Base {
        BINARY = 2,
        OCTAL = 8,
        DECIMAL = 10,
        HEXADECIMAL = 16
    };
    const char *base_to_string(Base base) {
        switch (base) {
            case BINARY: return "binary";
            case OCTAL: return "octal";
            case DECIMAL: return "decimal";
            case HEXADECIMAL: return "hexadecimal";
            default: return "(error -- unknown base to convert)";
        }
    }
};

Scanner::Scanner(std::string &str, Output &output) : str(str), output(output) {};

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
bool Scanner::skip_whitespace() {
    char curr = this->current();
    bool skipped = false;

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
        skipped = true;
    }
    return skipped;
}
bool Scanner::skip_comment() {
    if (this->current() == '/' && this->peek(1) == '/') {
        while (this->current() != '\n' && !this->at_EOF()) this->advance();
        return true;
    }
    if (this->current() == '/' && this->peek(1) == '*') {
        while (this->current() != '*' || this->peek(1) != '/') {
            if (this->at_EOF()) {
                this->output.error(
                    TokenPosition{ .line = this->line, .col = this->col, .length = 1 },
                    "Multiline /* comment wasn't terminated with a\"'*/\"",
                    Errors::LEX_ERROR);
                break;
            }
            this->advance();
        }
        // Go past the */
        this->advance();
        this->advance();
        return true;
    }

    return false;
}

TokenPosition Scanner::make_single_line_position(int length) const {
    return TokenPosition{ .line = this->line, .col = this->col - length, .length = length };
};

std::string* Scanner::parse_string() {
    char delimeter = this->advance();

    std::string* str = new std::string();

    bool continue_loop = true;

    while (continue_loop && this->current() != delimeter) {
        char curr = this->advance();

        /* Check and warn for trigraphs */
        if (curr == '?' && this->current() == '?') {
            // If this is still \0, that means that we didn't find a trigraph
            // and didn't have to warn the user
            char trigraph = '\0';
            switch (this->peek(1)) {
                case '=':  trigraph = '#';  break;
                case '/':  trigraph = '\\'; break;
                case '\'': trigraph = '^';  break;
                case '!':  trigraph = '|';  break;
                case '-':  trigraph = '~';  break;
                case '(':  trigraph = '[';  break;
                case ')':  trigraph = ']';  break;
                case '<':  trigraph = '{';  break;
                case '>':  trigraph = '}';  break;
            }

            if (trigraph != '\0') {
                char warning[100];
                snprintf(warning, 100, "Trigraphs are not supported. Write '%c' instead.", trigraph);
                this->output.warning(
                    TokenPosition{ .line = this->line, .col = this->col - 1, .length = 3 },
                    warning
                );
            }
        }

        // This should be updated depending on the place in the string.
        // Up to 4 UTF-8 bytes will be inserted, depending on the buffer
        // If it remains the null-terminating character, it is assumed that there
        // was an error. The character is then not appended.
        char buffer[4] = { 0 };
        // And this is the number of codepoints to be inserted.
        int length = 0;

        switch (curr) {
            case '\\':
            {
                char next = this->advance();
                if (next == '\0') {
                    this->output.error(
                        TokenPosition{ .line = this->line, .col = this->col, .length = 1 },
                        "Expected additional input after escape character '\\'.",
                        Errors::LEX_ERROR);
                    continue_loop = false;
                    break;
                }
                switch (next) {
                    case 'a':  buffer[0] = '\a'; length = 1; break;
                    case 'b':  buffer[0] = '\b'; length = 1; break;
                    case 'f':  buffer[0] = '\f'; length = 1; break;
                    case 'n':  buffer[0] = '\n'; length = 1; break;
                    case 'r':  buffer[0] = '\r'; length = 1; break;
                    case 't':  buffer[0] = '\t'; length = 1; break;
                    case 'v':  buffer[0] = '\v'; length = 1; break;
                    case '\\': buffer[0] = '\\'; length = 1; break;
                    case '\'': buffer[0] = '\''; length = 1; break;
                    case '"':  buffer[0] = '"';  length = 1; break;
                    // This is used to avoid trigraphs
                    case '?':  buffer[0] = '?';  length = 1; break;

                    // Octal escape sequence
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    {
                        char octal[3] = { 0 };
                        octal[0] = next;
                        
                        int octal_length = 1;
                        while (octal_length < 3) {
                            char next = this->current();
                            if (!is_octal_digit(next)) break;

                            // Max is \317, so make sure it's not greater than that
                            if (octal[0] > '3') break;
                            if (octal_length == 1 && octal[0] == '3' && next > '1') break;

                            octal[octal_length] = next;

                            this->advance();
                            octal_length += 1;
                        }

                        uint8_t byte = 0;
                        for (int digit = 0; digit < octal_length; digit += 1) {
                            byte += (octal[digit] - '0') * (1 << (3 * (octal_length - digit - 1)));
                        }
                        buffer[0] = static_cast<char>(byte);
                        length = 1;

                        // Go past 
                        for (int advance = 0; advance < octal_length - 1; advance += 1) this->advance();
                    }
                    break;
                    case 'x':
                        buffer[0] = this->parse_hex<2>("A \\x escape sequence requires two hexadecimal digits");
                        length = 1;
                        break;
                    case 'u':
                    {
                        uint32_t hex = this->parse_hex<4>("A \\u escape sequence requires four hexadecimal digits.");
                        length = utf32_codepoint_to_char_buffer(hex, buffer);
                    }
                        break;
                    case 'U':
                    {
                        uint32_t hex = this->parse_hex<8>("A \\U escape sequence requires eight hexadecimal digits.");
                        length = utf32_codepoint_to_char_buffer(hex, buffer);
                    }
                        break;

                    default:
                    {
                        char warning_message[60];
                        snprintf(warning_message, 60, "Escape character '\\' has no effect on the character '%c'", next);
                        this->output.warning(
                            TokenPosition{ .line = this->line, .col = this->col, .length = 1 },
                            warning_message);
                    }
                        buffer[0] = next;
                        length = 1;
                        break;
                }
            }
                break;

            default:
                buffer[0] = curr;
                length = 1;
                break;
        }
        for (int byte_ind = 0; byte_ind < length; byte_ind += 1) {
            str->append(1, buffer[byte_ind]);
        }
    }

    char delimiter_end = this->advance();
    if (delimiter_end == '\0') {
        this->output.error(
            TokenPosition{ .line = this->line, .col = this->col, .length = 1 },
            "Expected ending quote to string",
            Errors::LEX_ERROR);
    }

    return str;
}
Token Scanner::parse_number() {
    Base base = DECIMAL;
    if (this->current() == '0') {
        switch (this->peek(1)) {
            case 'b': base = BINARY; break;
            case 'o': base = OCTAL; break;
            case 'x': base = HEXADECIMAL; break;
            default: break;
        }
        // If we changed the base, go through those two characters
        if (base != DECIMAL) {
            this->advance();
            this->advance();
        }
    }
    const char *base_str = base_to_string(base);

    // Certain errors mean the number is no longer processable,
    // so we shouldn't add a number to the number string
    bool aggregate_characters = true;
    bool dec = false;

    // Number of characters we've consumed.
    // We need this because there are some characters we consume
    // (e.g. '_') that we can't include in the number string
    int length = 0;
    bool last_underscore = false;

    bool too_many_underscores = false;
    bool underscore_before_decimal = false;
    bool invalid_decimal_base = false;
    // If it changes from NULL, we see the invalid character
    char invalid_base_character = '\0';

    std::string number_string = "";

    int col = this->col;

    while (!this->at_EOF()) {
        char curr = this->current();

        if (curr == '.') {
            // We can't have two decimal places in one number, so if we've
            // already found one, exit
            if (dec) break;

            // If we only find a decimal after underscores, e.g., 4_.,
            // that's an error
            if (last_underscore) underscore_before_decimal = true;
            // We can only have a decimal point on a base-10 number,
            // so something like 0b11.001 isn't possible
            if (base != 10) {
                invalid_decimal_base = true;
                aggregate_characters = false;
            }

            else {
                number_string += '.';
                dec = true;
            }
        }
        else if (curr == '_') {
            if (last_underscore) too_many_underscores = true;
        }
        // If we didn't find a number or decimal place, break out of the loop
        else {
            if (
                is_digit(curr) ||
                (base == HEXADECIMAL && is_hex_digit(curr))
            ) {
                if (
                    (base == BINARY && !(curr == '0' || curr == '1')) ||
                    (base == OCTAL && !is_octal_digit(curr))
                ) {
                    invalid_base_character = curr;
                }
                else if (aggregate_characters) number_string += curr;
            }
            else break;
        };

        length += 1;
        last_underscore = curr == '_';
        // Only advance if it was a valid character.
        this->advance();
    }

    // We need the number for errors
    Values::number_t number;
    try {
        if (base != 10) {
            number = static_cast<Values::number_t>(std::stoi(number_string, 0, base));
        }
        else number = static_cast<Values::number_t>(std::stod(number_string));
    } catch (const std::exception&) {
        // If we catch an exception, that means the number failed to parse
        // That's an internal error and we should warn the user
        std::string error_message = "Lexer failed to parse number ";
        error_message += number_string;
        throw log_assert(error_message);
    };

    TokenPosition position = TokenPosition{ .line = this->line, .col = col, .length = length };
    Token error_tok = Token(number, position);

    if (too_many_underscores) {
        this->output.error(position, "Only one '_' digit seperator may appear in a row", Errors::LEX_ERROR);
        return error_tok;
    }
    if (underscore_before_decimal) {
        this->output.error(position, "Underscore may not appear directly before decimal point", Errors::LEX_ERROR);
        return error_tok;
    }
    if (last_underscore) {
        this->output.error(position, "Underscore may not appear at the end of number", Errors::LEX_ERROR);
        return error_tok;
    }
    if (invalid_decimal_base) {
        this->output.error(position, "A decimal point may only appear in a base-10 decimal number", Errors::LEX_ERROR);
        return error_tok;
    }
    if (invalid_base_character != '\0') {
        std::string error = "Invalid character '";
        error += invalid_base_character;
        error += "' in ";
        error += base_str;
        error += " number";
        this->output.error(position, error, Errors::LEX_ERROR);
    }

    return Token(number, this->make_single_line_position(length));
}

Token Scanner::next_token() {
    while (this->skip_whitespace() || this->skip_comment());

    /* If we're at the end of the file, just return an EOI.
        Make the length 1 so that errors saying they expected another
        token at the end of the file have a carat. */
    if (this->current() == '\0') {
        return Token(
            TokType::EOI,
            TokenPosition{ .line = this->line, .col = this->col, .length = 1 }
        );
    }

    char curr = this->current();

    // Constant symbols

    /* One or two character symbols */
    TokType one_char_type = TokType::ERROR;
    TokType two_char_type = TokType::ERROR;
    switch (curr) {
        case '+': one_char_type = TokType::PLUS; break;
        case '-': one_char_type = TokType::MINUS; break;
        case '*': one_char_type = TokType::STAR; break;
        case '/': one_char_type = TokType::SLASH; break;
        case '%': one_char_type = TokType::PERCENT; break;

        case '=': {
            if (this->peek(1) == '=') {
                two_char_type = TokType::EQUALS_EQUALS;
            }
            else one_char_type = TokType::EQUALS;
            break;
        }

        case '(': one_char_type = TokType::LPAREN; break;
        case ')': one_char_type = TokType::RPAREN; break;
        case '[': one_char_type = TokType::LBRACE; break;
        case ']': one_char_type = TokType::RBRACE; break;
        case '{': one_char_type = TokType::LBRACKET; break;
        case '}': one_char_type = TokType::RBRACKET; break;
        case '<': {
            if (this->peek(1) == '=') two_char_type = TokType::LESS_THAN_EQUAL;
            else one_char_type = TokType::LESS_THAN;
        }
            break;
        case '>': {
            if (this->peek(1) == '=') two_char_type = TokType::GREATER_THAN_EQUAL;
            else one_char_type = TokType::GREATER_THAN;
        }
            break;

        case '?': one_char_type = TokType::QUESTION_MARK; break;
        case ':': one_char_type = TokType::COLON; break;
        case ';': one_char_type = TokType::SEMICOLON; break;
        case ',': one_char_type = TokType::COMMA; break;
        case '.': one_char_type = TokType::DOT; break;
        case '!': {
            if (this->peek(1) == '=') two_char_type = TokType::BANG_EQ;
        }
            break;
    };
    if (one_char_type != TokType::ERROR) {
        this->advance();
        return Token(one_char_type, this->make_single_line_position(1));
    }
    else if (two_char_type != TokType::ERROR) {
        this->advance();
        this->advance();
        return Token(two_char_type, this->make_single_line_position(2));
    }

    // Number?
    if ( is_digit(curr) || (curr == '.' && is_digit(this->peek(1))) ) {
        return this->parse_number();
    }

    // String
    if ( curr == '"' ) {
        int start_ind = static_cast<uint>(this->ind),
            start_col = this->col,
            start_line = this->line;
        std::string* str = this->parse_string();
        return Token(TokType::STRING, str, TokenPosition{ .line = start_line, .col = start_col, .length = static_cast<int>(this->ind) - start_ind });
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
    char error_message[50];
    snprintf(error_message, 50, "Unknown character '%c'", curr);
    this->output.error(TokenPosition{ .line = this->line, .col = this->col, .length = 1 }, error_message, Errors::LEX_ERROR);

    this->advance();
    return Token(TokType::ERROR, TokenPosition{ .line = -1, .col = -1, .length = -1 });
}
Token Scanner::next_real_token() {
    while (true) {
        Token token = this->next_token();
        if (token.get_type() != TokType::ERROR) return token;
    }
}