#ifndef _SG_CPP_NATIVES_HPP
#define _SG_CPP_NATIVES_HPP

#include "../value.hpp"

#include <array>
#include <unordered_map>

namespace Natives {
    const int native_count = 2;
    /* Update this list when you add a new native, and remove it when you take out a native. */
    extern const char *native_names[native_count];

    struct Native {
        const char *native_name;
        Values::Value value;

        Native(const char *name, Values::Value value);
        /* Default constructor so it can be part of array class */
        inline Native() {};
    };

    extern std::unordered_map<std::string, uint> name_to_native_index;

    uint get_native_index(std::string native_name);
    void create_natives(std::array<Native, native_count> &natives);
};

#endif