#ifndef _SGCPP_UTILS_HPP
#define _SGCPP_UTILS_HPP

#include "globals.hpp"

#include <random>
#include <string>

/* Get the number of digits in a decimal number */
uint get_digits(uint number);

#ifdef DEBUG
std::string var_ind_to_subscript(int num);
uint get_string_length_as_utf32(std::string str);
#endif

uint64_t time_in_millis();

namespace Random {
    typedef std::mt19937 RNG;
    extern RNG rng;

    void initialize_rng();
}

// Given a Unicode codepoint, inserts the corresponding UTF-8 chunk into the buffer.
// Buffer MUST be initialized to all zeroes for expected behavior
// Returns the number of UTF-8 bytes inserted (1 - 4)
// Proudly stolen from Github user Miouyouyou. Thank you!
int utf32_codepoint_to_char_buffer(uint32_t codepoint, char buffer[4]);

#endif