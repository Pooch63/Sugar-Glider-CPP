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

        case LPAREN: return "(";
        case RPAREN: return ")";
        case LBRACKET: return "{";
        case RBRACKET: return "}";

        case QUESTION_MARK: return "?";
        case COLON: return ":";
        case SEMICOLON: return ";";

        case BREAK: return "break";
        case CONST: return "const";
        case CONTINUE: return "continue";
        case FALSE: return "false";
        case IF: return "if";
        case TRUE: return "true";
        case VAR: return "var";
        case WHILE: return "while";

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
    bool is_octal_digit(char c) { return c >= '0' && c <= '7'; }

    bool is_identifier_start(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    /* Can it go after the starting character in an identifier */
    bool is_identifier_middle(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    static std::unordered_map<std::string, TokType> keywords = {
        { "break", TokType::BREAK },
        { "const", TokType::CONST },
        { "continue", TokType::CONTINUE },
        { "false", TokType::FALSE },
        { "if", TokType::IF },
        { "true", TokType::TRUE },
        { "var", TokType::VAR },
        { "while", TokType::WHILE },
    };
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
                        "Expected additional input after escape character '\\'.");
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
            "Expected ending quote to string" );
    }

    return str;
}

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
    this->output.error(TokenPosition{ .line = this->line, .col = this->col, .length = 1 }, error_message);

    this->advance();
    return Token(TokType::ERROR, TokenPosition{ .line = -1, .col = -1, .length = -1 });
}
Token Scanner::next_real_token() {
    while (true) {
        Token token = this->next_token();
        if (token.get_type() != TokType::ERROR) return token;
    }
}