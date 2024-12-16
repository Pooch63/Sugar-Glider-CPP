#ifndef _SGCPP_ERRORS_HPP
#define _SGCPP_ERRORS_HPP

#include <string>
#include <vector>

namespace Colors {
    enum Color {
        RED = 31,
        YELLOW = 33,
        BLUE = 34,
        PURPLE = 35,
        WHITE = 37,
        MAGENTA = 95,

        DEFAULT = 0
    };
};
namespace Position {
    struct TokenPosition {
        /* 0-indexed */
        int line;
        /* 0-indexed */
        int col;
        /* How many characters long it is */
        int length;
    };
    /* Null token information for an error token */
    const TokenPosition null_token_position = TokenPosition{ .line = -1, .col =-1, .length = -1 };
};
namespace Output {
    /* Generate a list of the indices of all the newlines in a program.
        This is so we know where lines begin and end, which we need when
        logging errors. */
    void extract_line_ends(std::vector<int>& line_ends, std::string& prog);


};

#endif