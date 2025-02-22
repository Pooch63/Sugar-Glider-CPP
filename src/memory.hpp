#ifndef _SG_CPP_MEMORY_HPP
#define _SG_CPP_MEMORY_HPP

#include "errors.hpp"

#include <new>

template <typename T>
class Allocate {
public:
    template <typename... Args>
    static T *create(Args &&...args) {
        try {
            return new T(std::forward<Args>(args)...);
        } catch (const std::bad_alloc&) {
            throw memory_error();
        }
    }
};

#endif