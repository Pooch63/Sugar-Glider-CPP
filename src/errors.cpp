#include "errors.hpp"

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
void Output::output_line(Position::TokenPosition position, rang::fg problem_color, std::ostream &output) {
    using std::max, std::min;

    // Log the line information
    // Add one to line and column since they're both 0-indexed
    std::string location = std::to_string(position.line + 1);
    location += ':';
    location += std::to_string(position.col + 1);
    location += " |  ";

    output << rang::fg::yellow << location << rang::style::reset;

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
        output << this->prog.at(ind);
    }
    /* Now, log the area color. */
    std::cout << problem_color;
    // An EOF token goes past the program length, so make sure not to go too far
    for (int ind = error_start; ind < min(
            static_cast<int>(this->prog.size()), error_start + position.length
        ); ind += 1) {
        output << this->prog.at(ind);
    }
    std::cout << rang::style::reset;

    /* Finally, log characters to the right of the error */
    int right_characters_start = error_start + position.length;
    for (int ind = right_characters_start;
        ind <= min(left_start + Output::MAX_LOG_LENGTH, line_end);
        ind += 1) {
            output << this->prog.at(ind);
    }
    // Now, log the characters underneath
    output << '\n';

    /* Add spaces until we get underneath the error start */
    for (uint space = 0; space < location.size() + position.col - start_col; space += 1) {
        output << ' ';
    }
    /* Add the carats */
    output << rang::fg::magenta;
    for (int carat = 0; carat < position.length; carat += 1) {
        output << '^';
    }
    output << '\n';
};
void Output::error(Position::TokenPosition position, std::string error, Errors::ErrorCode code) {
    this->output_line(position, rang::fg::red, std::cerr);

    std::cerr << rang::style::bold << rang::fg::red << "error: ";
    std::cerr << rang::style::reset << error;
    std::cerr << '\n' << std::endl;

    this->error_code = code;
}
void Output::warning(Position::TokenPosition position, std::string warning) {
    this->output_line(position, rang::fg::yellow, std::cout);

    std::cerr << rang::style::bold << rang::fg::red << "warning: ";
    std::cerr << rang::style::reset << warning;
    std::cerr << '\n' << std::endl;
}

std::runtime_error log_assert(std::string message) {
    std::cerr << '\n' << rang::style::bold << rang::fg::red << "internal error\n";
    std::cerr << rang::style::reset << "Please report this issue\n";
    return std::runtime_error(message);
}
std::runtime_error memory_error() {
    std::cerr << '\n' << rang::style::bold << rang::fg::red << "memory error\n";
    return std::runtime_error("Failed to allocate memory. Self-kill.");
}