#ifndef _SGCPP_LEXER_HPP
#define _SGCPP_LEXER_HPP

#include "../errors.hpp"
#include "../globals.hpp"
#include "../value.hpp"

#include <memory>
#include <string>
#include <vector>

using Position::TokenPosition;

namespace Scan {
    enum TokType {
        // Values >
        NUMBER,
        STRING,
        // < Values

        IDENTIFIER,

        /* The rest of the token types are constants.
            Their value will always be the same. (e.g., +, *, /, ], etc.) */
        // Operators >
        PLUS,
        MINUS,
        STAR,
        SLASH,
        PERCENT,

        // =
        EQUALS,

        // (
        LPAREN,
        // )
        RPAREN,
        // {
        LBRACKET,
        // }
        RBRACKET,
        // ?
        QUESTION_MARK,
        // :
        COLON,
        // ;
        SEMICOLON,
        // < Operators

        // Keywords >
        BREAK,
        CONST,
        CONTINUE,
        FALSE,
        IF,
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
                a non-whitespace character */
            void skip_whitespace();

            std::string &str;
            Output &output;

        public:
            Scanner(std::string& str, Output &output);

            Token next_token();
            /* Get the next actual token, since next_token will return a ghost "error" token
                if it encounters a non-recoverable input error. */
            Token next_real_token();
    };
}

#endif