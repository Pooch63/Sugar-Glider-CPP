#ifndef _SG_CPP_TIME_UTIL_HPP
#define _SG_CPP_TIME_UTIL_HPP

#include "globals.hpp"

#include <string>

uint64_t time_in_nanoseconds();
uint64_t time_in_millis();

std::string_view get_timezone_offset_name();

#endif