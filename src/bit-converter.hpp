/* Helper functions to manipulate bits and different types,
    following the endian-ness of the system.
    Big Endian has not been tested yet. */

#ifndef _SGCPP_BIT_CONVERTER_HPP
#define _SGCPP_BIT_CONVERTER_HPP

#include <cstdint>

template<typename T>
uint8_t get_bottom_byte(T value) {
    #ifdef _SGCPP_SYSTEM_IS_BIG_ENDIAN
    return static_cast<T>(value & (~static_cast<T>(255) << (sizeof(T) - 8)));
    #else
    return static_cast<uint8_t>(value & 255);
    #endif
};

#endif