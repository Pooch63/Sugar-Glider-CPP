#ifndef _SGCPP_GLOBALS_HPP
#define _SGCPP_GLOBALS_HPP

#define DEBUG
#define DEBUG_ASSERT // run every single assertion to make sure the program is working.
#define DEBUG_GC
#define DEBUG_STRESS_GC // run garbage collector after every allocation

#define IR_LABEL_LENGTH 20 // length of label name in IR. Reduce for memory-tight constraints, but too small and label collisions will occur.
#define MAX_FUNCTION_ARGUMENTS 255
#define MAX_CALL_STACK_SIZE 40 * 1024 // in bytes
#define STRINGIFY(x) #x

#ifdef DEBUG
    #include <iostream>
#endif

#include "errors.hpp"
/* Used for asserts that just log a nice message
Mostly used for asserts that are at the end of functions.
Use throw sg_assert_error("...") to avoid triggering a warning or error
for reaching the end of control flow without returning. */
#define sg_assert_error(message) log_assert(message)

#include <cstdint>

typedef unsigned int uint;

namespace Values {
    typedef double number_t;
};

#endif