#ifndef _SGCPP_GLOBALS_HPP
#define _SGCPP_GLOBALS_HPP

#define DEBUG
#define DEBUG_ASSERT // run every single assertion to make sure the program is working. Certain assertions may still be run if DEBUG is set

#ifdef DEBUG
    #include <iostream>
#endif

#include <cstdint>

typedef unsigned int uint;

#endif