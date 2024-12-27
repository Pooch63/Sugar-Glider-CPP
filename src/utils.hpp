#ifndef _SGCPP_UTILS_HPP
#define _SGCPP_UTILS_HPP

#include "globals.hpp"

#include <random>
#include <string>

/* Get the number of digits in a decimal number */
uint get_digits(uint number);

#ifdef DEBUG
std::string number_as_subscript(uint num);
uint get_string_length_as_utf32(std::string str);
#endif

uint64_t time_in_millis();

namespace Random {
    typedef std::mt19937 RNG;
    extern RNG rng;

    void initialize_rng();
}

#endif