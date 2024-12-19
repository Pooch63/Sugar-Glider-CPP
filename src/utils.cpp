#include "utils.hpp"

uint get_digits(uint number) {
    return number < 10 ? 1 :
            number < 100 ? 2 :
            number < 1'000 ? 3 :
            number < 10'000 ? 4 :
            number < 100'000 ? 5 :
            number < 1'000'000 ? 6 :
            number < 10'000'000 ? 7 :
            number < 100'000'000 ? 8 : 9;
}