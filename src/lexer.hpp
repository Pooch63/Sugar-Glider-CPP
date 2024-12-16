#ifndef _SGCPP_LEXER_HPP
#define _SGCPP_LEXER_HPP

#include "errors.hpp"
#include "globals.hpp"
#include "value.hpp"

#include <string>
#include <vector>

using Position::TokenPosition;

namespace Scan {
    enum TokType {
        NUMBER,

        /* The rest of the token types are constants.
            Their value will always be the same. (e.g., +, *, /, ], etc.) */
        PLUS,
        MINUS,
        STAR,
        SLASH,
        PERCENT,

        /* End of input. Signifies that there were no more tokens. */
        EOI,

        /* Not a real token type. Instead, it is returned when there is a lexer error
            and we can't return a real token. */
        ERROR,

        /* Just a value at the end to get the number of token types */
        NUM_TOKEN_TYPES
    };
    union token_payload_t {
        Value::number_t num;
    };

    class Token {
        /* Payload is only valid if the type calls for it. E.g, if the token type is PLUS,
            that value is a constant, so the payload's number can be anything.
            Do not read members of the payload if the token type is a constant (e.g., +, *, (, ]). */
        private:
            TokType type;
            token_payload_t payload;
            TokenPosition position;
        public:
            /* Use this constructor if the token type is a constant and does not need a payload. */
            Token(TokType type, TokenPosition position);
            /* Use this constructor if the token type is a number. The token type will automatically be set. */
            Token(Value::number_t number, TokenPosition position);

            inline TokType get_type() const { return this->type; };
            Value::number_t get_number() const;

            #ifdef DEBUG
            std::string to_string() const;
            #endif
    };

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
            // Skip all whitespace characters, leaving the index at the next instance of
            // a non-whitespace character
            void skip_whitespace();

            std::string& str;

        public:
            Scanner(std::string& str);

            Token next_token();
    };
}

#endif