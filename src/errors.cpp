#include "errors.hpp"

#include <iostream>


std::string Colors::create_color(Color color) {
    std::string str = "\x1b[";
    str += std::to_string(static_cast<int>(color));
    str += 'm';
    return str;
};
void Colors::set_color(Color color) {
    std::cout << create_color(color);
}

/* Automatically extracts line ends. */
Output::Output(std::string& prog) : prog(prog) {
    this->extract_line_ends();
}
void Output::extract_line_ends() {
    for (int index = 0; static_cast<uint>(index) < this->prog.size(); index += 1) {
        if (this->prog[index] == '\n') this->line_ends.push_back(index);
    }
    // We need a line end for the VERY end of the program
    this->line_ends.push_back(prog.size());
}
int Output::get_line_end(int line) const {
    return line == -1 ? -1 : this->line_ends.at(line);
}
void Output::output_line(Position::TokenPosition position, Colors::Color problem_color) {
    using std::max, std::min;

    this->errored = true;

    // Log the line information
    // Add one to line and column since they're both 0-indexed
    std::string location = std::to_string(position.line + 1);
    location += ':';
    location += std::to_string(position.col + 1);
    location += " |  ";

    Colors::set_color(Colors::YELLOW);
    std::cout << location;
    Colors::set_color(Colors::DEFAULT);

    int line_start = this->get_line_end(position.line - 1) + 1;
    int line_end = this->get_line_end(position.line) - 1;

    int start_col = max(position.col - Output::MAX_LEFT_CHARACTERS, 0);
    /* The index of the place in the program where we'll start logging
        to the left of the message. We have to log characters on the same line. */
    int left_start = line_start + start_col;
    /* The index of the start of the error section. */
    int error_start = line_start + position.col;

    // Log the line
    /* First, log the characters to the left */
    for (int ind = left_start; ind < error_start; ind += 1) {
        std::cout << this->prog.at(ind);
    }
    /* Now, log the area color. */
    Colors::set_color(problem_color);
    // An EOF token goes past the program length, so make sure not to go too far
    for (int ind = error_start; ind < min(
            static_cast<int>(this->prog.size()), error_start + position.length
        ); ind += 1) {
        std::cout << this->prog.at(ind);
    }
    Colors::set_color(Colors::DEFAULT);

    /* Finally, log characters to the right of the error */
    int right_characters_start = line_start + error_start + position.length;
    for (int ind = right_characters_start;
        ind <= min(line_start + Output::MAX_LOG_LENGTH, line_end);
        ind += 1) {
            std::cout << this->prog.at(ind);
    }
    // Now, log the characters underneath
    std::cout << '\n';

    /* Add spaces until we get underneath the error start */
    for (uint space = 0; space < location.size() + position.col - start_col; space += 1) {
        std::cout << ' ';
    }
    /* Add the carats */
    Colors::set_color(Colors::MAGENTA);
    for (int carat = 0; carat < position.length; carat += 1) {
        std::cout << '^';
    }
    std::cout << '\n';
};
void Output::error(Position::TokenPosition position, std::string error) {
    this->output_line(position, Colors::RED);

    Colors::set_color(Colors::BOLD);
    Colors::set_color(Colors::RED);
    std::cout << "error: ";

    Colors::set_color(Colors::DEFAULT);
    std::cout << error;


    std::cout << '\n' << std::endl;
}
void Output::warning(Position::TokenPosition position, std::string warning) {
    this->output_line(position, Colors::YELLOW);

    Colors::set_color(Colors::BOLD);
    Colors::set_color(Colors::MAGENTA);
    std::cout << "warning: ";

    Colors::set_color(Colors::DEFAULT);
    std::cout << warning;

    std::cout << '\n' << std::endl;
}