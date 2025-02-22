#ifndef _SGCPP_LEXER_HPP
#define _SGCPP_LEXER_HPP

#include "../errors.hpp"
#include "../globals.hpp"
#include "../value.hpp"

#ifdef DEBUG_ASSERT
#include <cassert>
#endif

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using Position::TokenPosition;

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
inline bool is_hex_digit(char c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); };
static uint8_t hex_digit_to_number(char c) {
    if (is_digit(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    throw sg_assert_error("unknown hexadecimal digit");
}

namespace Scan {
    enum TokType {
        // Values >
        NUMBER,
        STRING,
        // < Values

        IDENTIFIER,

        /* The rest of the token types are constants.message_on_error
            Their value will always be the same. (e.g., +, *, /, ], etc.) */
        // Operators >
        PLUS,
        MINUS,
        STAR,
        SLASH,
        PERCENT,

        // =
        EQUALS,
        // ==
        EQUALS_EQUALS,
        // !=
        BANG_EQ,

        // (
        LPAREN,
        // )
        RPAREN,
        // [
        LBRACE,
        // ]
        RBRACE,
        // {
        LBRACKET,
        // }
        RBRACKET,
        // <
        LESS_THAN,
        // >
        GREATER_THAN,
        // ?
        QUESTION_MARK,
        // :
        COLON,
        // ;
        SEMICOLON,
        // ,
        COMMA,
        // .
        DOT,
        // < Operators

        // Keywords >
        BREAK,
        CONST,
        CONTINUE,
        FALSE,
        FUNCTION,
        IF,
        NULL_TOKEN,
        RETURN,
        TRUE,
        VAR,
        WHILE,
        // < Keywords

        /* End of input. Signifies that there were no more tokens. */
        EOI,

        /* Not a real token type. Instead, it is returned when there is a lexer error
            and we can't return a real token. */
        ERROR,
        /* Not a real token type. Used by the parser as padding at the start */
        START_DELIMETER,

        /* Just a value at the end to get the number of token types */
        NUM_TOKEN_TYPES
    };
    union token_payload_t {
        Values::number_t num;
        /* Used for identifiers or strings */
        std::string* string_;
    };
    const char* tok_type_to_string(TokType type);

    class Token {
        /* Payload is only valid if the type calls for it. E.g, if the token type is PLUS,
            that value is a constant, so the payload's number can be anything.
            Do not read members of the payload if the token type is a constant (e.g., +, *, (, ]). */
        private:
            TokType type;
            token_payload_t payload;
            TokenPosition position;
            /* Whether or not we should free the payload. */
            bool free_payload = true;

            inline bool has_string_payload() const {
                return this->type == TokType::IDENTIFIER || this->type == TokType::STRING;
            };
        public:
            /* Use this constructor if the token type is a constant and does not need a payload. */
            Token(TokType type, TokenPosition position);
            /* Use this constructor if the token type is a number. The token type will automatically be set to number. */
            Token(Values::number_t number, TokenPosition position);
            /* Use this constructor if the token type is a string. Used for identifiers or strings. */
            Token(TokType type, std::string* str_, TokenPosition position);

            inline TokType get_type() const { return this->type; };
            inline TokenPosition get_position() const { return this->position; };

            Values::number_t get_number() const;
            std::string* get_string() const;

            #ifdef DEBUG
            std::string to_string() const;
            #endif

            /* The free method WILL free the payload, unless you call this method.
                Used to reserve things like tokens with string payloads that need to be moved. */
            inline void mark_payload() { this->free_payload = false; };

            void free();
            // ~Token();
    };
    const Token start = Token(TokType::START_DELIMETER, TokenPosition{ .line = 0, .col = 0, .length = 0 });

    /* Use std::string because we don't know how long it could be */
    std::string tok_to_concise_string(Token token);

    class Scanner {
        private:
            // Position variables
            /* 0-indexed */
            int line = 0;
            /* 0-indexed. */
            int col = 0;
            /* Position in the program */
            uint ind = 0;

            // Position helpers
            /* Get the current character.
                If we're past the end of the program, return a null-terminating character */
            char current() const;
            /* Increase the program index by one, and return the character before advancing.
                If we were already at the end of the program, instead return 0x00 null-termiating character. */
            char advance();
            /* Return the digit at skip_count characters after the current index.
                If skip_count goes past the end of the string, return 0x00 null-terminating character. */
            char peek(int skip_count) const;
            /* Returns whether or not the current index goes past the end of the string. */
            bool at_EOF() const;

            /* If the current column is the last character of a token that is all on the same line,
                then make a token position instance accordingly. */
            TokenPosition make_single_line_position(int length) const;

            // Other helpers
            /* Skip all whitespace characters, leaving the index at the next instance of
                a non-whitespace character. Return whether or not whitespace was skipped. */
            bool skip_whitespace();
            /* Skip the first comment you find, leaving the index at the next character after
                the comment. Return whether or not a comment was skipped. */
            bool skip_comment();

            template<int num_hex_digits>
            using hex_payload = typename std::conditional<
                num_hex_digits <= 2,
                char,
                char32_t >::type;
            /* Read in the specified number of hex digits and return the number. */
            template<uint num_hex_digits>
            hex_payload<num_hex_digits> parse_hex(std::string message_on_error) {
                uint64_t payload = 0;

                for (uint hex_ind = 0; hex_ind < num_hex_digits; hex_ind += 1) {
                    char curr = this->current();

                     if (!is_hex_digit(curr)) {
                        this->output.error(
                            TokenPosition{ .line = this->line, .col = this->col, .length = static_cast<int>(num_hex_digits - hex_ind) },
                            message_on_error,
                            Errors::LEX_ERROR
                        );
                        return 0;
                    }

                    payload |= static_cast<uint64_t>(hex_digit_to_number(curr)) << (4 * (num_hex_digits - hex_ind - 1));
                    this->advance();
                }

                return static_cast<hex_payload<num_hex_digits>>(payload);
            }

            /* Should be called while the current character is the delimeter,
                e.g., current is "" or ' */
            std::string* parse_string();
            /* Should be called when the first character is a '.' or a digit */
            Token parse_number();

            std::string &str;
            Output &output;

            Token next_token();

        public:
            Scanner(std::string& str, Output &output);

            /* Get the next actual token, since next_token will return a ghost "error" token
                if it encounters a non-recoverable input error. */
            Token next_real_token();
    };
}

#endif