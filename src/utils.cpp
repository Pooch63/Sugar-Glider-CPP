#include "utils.hpp"

#include <chrono>

#ifdef DEBUG
#include <cassert>
#include <iostream>
#endif

uint get_digits(uint number) {
    return number < 10 ? 1 :
            number < 100 ? 2 :
            number < 1'000 ? 3 :
            number < 10'000 ? 4 :
            number < 100'000 ? 5 :
            number < 1'000'000 ? 6 :
            number < 10'000'000 ? 7 :
            number < 100'000'000 ? 8 : 9;
}

static const char subscripts[10][4] = {
    "\u2080",
    "\u2081",
    "\u2082",
    "\u2083",
    "\u2084",
    "\u2085",
    "\u2086",
    "\u2087",
    "\u2088",
    "\u2089"
};

std::string var_ind_to_subscript(int num) {
    std::string str = std::to_string(abs(num));
    std::string output = "";

    if (num < 0) output += '-';

    for (char c : str) {
        uint subscript_ind = c - '0';
        // If it's not a digit, throw
        if (subscript_ind >= 10) throw sg_assert_error("Tried to convert non-digit character to subscript");

        output += subscripts[subscript_ind];
    }

    return output;
};

void truncate_string(std::string &output, uint max_len, std::string &value) {
    #ifdef DEBUG_ASSERT
    assert(max_len >= 3);
    #endif

    if (value.size() <= max_len) output = value;
    else {
        output = value.substr(0, max_len - 3);
        output += "...";
    }
};

/* Count code points in a UTF-8 string.
    Proudly stolen from Marcelo Cantos on https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring. */
uint get_string_length_as_utf32(std::string str) {
    const char *s = str.c_str();
    int len = 0;
    while (*s) len += (*s++ & 0xc0) != 0x80;
    return len;
}

uint64_t time_in_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

Random::RNG Random::rng;

void Random::initialize_rng() {
    rng.seed(time_in_millis());
}

int utf32_codepoint_to_char_buffer(uint32_t codepoint, char buffer[4]) {
    if (codepoint < 0x80) {
        buffer[0] = codepoint;
        return 1;
    }
    else if (codepoint < 0x800) {   // 00000yyy yyxxxxxx
        buffer[0] = (0b11000000 | (codepoint >> 6));
        buffer[1] = (0b10000000 | (codepoint & 0x3f));
        return 2;
    }
    else if (codepoint < 0x10000) {  // zzzzyyyy yyxxxxxx
        buffer[0] = (0b11100000 | (codepoint >> 12));         // 1110zzz
        buffer[1] = (0b10000000 | ((codepoint >> 6) & 0x3f)); // 10yyyyy
        buffer[2] = (0b10000000 | (codepoint & 0x3f));        // 10xxxxx
        return 3;
    }
    else if (codepoint < 0x200000) { // 000uuuuu zzzzyyyy yyxxxxxx
        buffer[0] = (0b11110000 | (codepoint >> 18));          // 11110uuu
        buffer[1] = (0b10000000 | ((codepoint >> 12) & 0x3f)); // 10uuzzzz
        buffer[2] = (0b10000000 | ((codepoint >> 6) & 0x3f)); // 10yyyyyy
        buffer[3] = (0b10000000 | (codepoint & 0x3f));         // 10xxxxxx
        return 4;
    }

    throw sg_assert_error("Unknown codepoint given to utf32_codepoint_to_char_buffer function");
    return -1;
}