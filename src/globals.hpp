#ifndef _SGCPP_GLOBALS_HPP
#define _SGCPP_GLOBALS_HPP

#define DEBUG
#define DEBUG_ASSERT // run every single assertion to make sure the program is working. Certain assertions may still be run if DEBUG is set

#define IR_LABEL_LENGTH 20 // length of label name in IR. Reduce for memory-tight constraints, but too small and label collisions will occur.

#ifdef DEBUG
    #include <iostream>
#endif

#include <cstdint>

typedef unsigned int uint;

#endif