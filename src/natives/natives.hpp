#ifndef _SG_CPP_NATIVES_HPP
#define _SG_CPP_NATIVES_HPP

#include "../value.hpp"

#include <array>
#include <unordered_map>

#define NATIVE_FUNCTION_HEADERS() ( \
    [[maybe_unused]] const Value * const stack, \
    [[maybe_unused]] uint stack_size, \
    [[maybe_unused]] Value &result, \
    [[maybe_unused]] Runtime &runtime, \
    [[maybe_unused]] std::string &error_message)

namespace Natives {
    const int native_count = 4;

    struct Native {
        const char *native_name;
        Values::Value value;

        Native(const char *name, Values::Value value);
        /* Default constructor so it can be part of array class */
        inline Native() {};
    };

    extern std::unordered_map<std::string, uint> name_to_native_index;

    uint get_native_index(std::string native_name);
    void create_natives(std::array<Values::Value, native_count> &natives);
};

#endif