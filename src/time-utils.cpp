#include "time-utils.hpp"

#include <ctime>
#include <chrono>

using namespace std;

template<typename chronos_type>
static uint64_t get_time() {
    return chrono::duration_cast<chronos_type>(
        chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

uint64_t time_in_nanoseconds() {
    return get_time<chrono::nanoseconds>();
}
uint64_t time_in_millis() {
    return get_time<chrono::milliseconds>();
}

std::string_view get_timezone_offset_name() {
    return chrono::current_zone()->name();
}