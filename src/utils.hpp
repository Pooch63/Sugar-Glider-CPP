#ifndef _SGCPP_UTILS_HPP
#define _SGCPP_UTILS_HPP

#include "globals.hpp"

/* Get the number of digits in a decimal number */
uint get_digits(uint number);

#ifdef DEBUG
void log_number_as_subscript(uint num);
#endif

#endif