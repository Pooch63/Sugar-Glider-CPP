#include "utils.hpp"

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

void log_number_as_subscript(uint num) {
    std::string str = std::to_string(num);

    for (char c : str) {
        uint subscript_ind = c - '0';
        // If it's not a digit, assert false
        if (subscript_ind >= 10) assert(false);

        std::cout << subscripts[subscript_ind];
    }
};
