#ifndef _SGCPP_ERRORS_HPP
#define _SGCPP_ERRORS_HPP

#include <string>
#include <vector>

namespace Colors {
    enum Color {
        /* Colors */
        RED = 31,
        YELLOW = 33,
        BLUE = 34,
        PURPLE = 35,
        WHITE = 37,
        MAGENTA = 95,

        /* Text styles */
        BOLD = 1,

        DEFAULT = 0,
    };

    /* Set the terminal to the color */
    void set_color(Color color);
};
namespace Position {
    struct TokenPosition {
        /* These values are integers because for certain line output calculations,
            we'll need to have temporary negative values. */

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
class Output {
    private:
        // Error modifiers
        /* The max number of characters we log to the left of an error. */
        static const int MAX_LEFT_CHARACTERS = 10;
        /* The max length of a line we log as an error. */
        static const int MAX_LOG_LENGTH = 50;

        /* A list of the indices of all the newlines in a program.
            This is so we know where lines begin and end, which we need when
            logging errors. It also has an index that points one PAST the end
            of the program. */
        std::vector<int> line_ends = std::vector<int>();
        std::string& prog;
        
        /* Extract the program's line ends into the line_ends vector */
        void extract_line_ends();

        /* Get the index of the newline character at the given line.
            If -1 is passed, -1 is returned (since it's the index of the character
            right before the start of the program.) */
        int get_line_end(int line);

    public:
        /* Automatically extract line inds. */
        Output(std::string& prog);

        /* Write an error to the console */
        void error(Position::TokenPosition position, std::string error);
};

#endif