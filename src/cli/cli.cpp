#include "pipeline.hpp"

#include "../../lib/rang.hpp"

#include <fstream>
#include <iostream>

// To store something like sgr run [script]
class Use {
    public:
        // E.g., run
        std::string name;
        // E.g., [script]
        std::string additional;
        // E.g., run a script
        std::string description;
        Use(std::string name, std::string additional, std::string description) :
            name(name), additional(additional), description(description) {};
};
#define USE_COUNT 1
Use uses[USE_COUNT] = {
    Use("run", "[script]", "Compile and run a script at the given path")
};

static void cli_error(std::string error) {
    std::cout << rang::style::bold << rang::fg::red << "error: " << rang::style::reset << error << '\n';
}
static bool cli_read_file(std::string path, std::string &buffer) {
    std::ifstream file(path);
    if (file.is_open()) {
        while (std::getline(file, buffer));
        file.close();
        return true;
    }
    else {
        cli_error("Could not read file " + path);
        return false;
    }
}

static const int use_name_length = 10;
static const int use_additional_length = 10;
static void cli_show_help_message() {
    std::cout << "Sugar Glider CLI usage\n";
    std::cout << "\tA CLI to run Sugar Glider, a dynamically typed programming language\n\n";

    /* First, log the uses */
    for (int ind = 0; ind < USE_COUNT; ind += 1) {
        Use use = uses[ind];
        std::cout << rang::fg::yellow << "sgr " << rang::style::reset << rang::style::bold << rang::fg::blue << use.name << rang::style::reset;
        for (int space = use.name.size(); space < use_name_length; space += 1) {
            std::cout << ' ';
        }
        std::cout << use.additional;
        for (int space = use.additional.size(); space < use_additional_length; space += 1) {
            std::cout << ' ';
        }
        std::cout << "- " << use.description << '\n';
    }

    std::cout << '\n';
}
static int cli_run_program(int argc, char **argv) {
    /* Need exactly three arguments */
    if (argc != 3) {
        if (argc < 3) cli_error("sgr run requires a path argument");
        else cli_error("sgr run was given too many arguments");
        cli_show_help_message();
        return -1;
    }

    std::string file;
    bool valid = cli_read_file(argv[2], file);

    if (!valid) return -1;

    return run_file(file);
}

int process_cli_arguments(int argc, char **argv) {
    if (argc == 1) {
        cli_show_help_message();
        return 0;
    }
    else if (std::string(argv[1]) == "run") {
        return cli_run_program(argc, argv);
    }

    return 0;
}