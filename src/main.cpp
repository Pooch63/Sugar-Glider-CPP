#include "cli/cli.hpp"
#include "utils.hpp"

/* I'm keeping all the library includes for nostalgic purposes. I'm weird :)
#include "compiler/ast.hpp"
#include "compiler/compiler.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "errors.hpp"
#include "ir/bytecode.hpp"
#include "ir/intermediate.hpp"
#include "ir/transpiler.hpp"
#include "optimizer/label-intermediate.hpp"
*/

#include <iostream>

int main(int argc, char **argv) {
    Random::initialize_rng();

    return process_cli_arguments(argc, argv);
}