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

#ifdef DEBUG
std::string number_as_subscript(uint num) {
    std::string str = std::to_string(num);
    std::string output = "";

    for (char c : str) {
        uint subscript_ind = c - '0';
        // If it's not a digit, throw
        if (subscript_ind >= 10) assert(false);

        output += subscripts[subscript_ind];
    }

    return output;
};
#endif

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