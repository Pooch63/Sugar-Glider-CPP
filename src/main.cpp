#include "ast.hpp"
#include "bytecode.hpp"
#include "compiler.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>

int main() {
    Parse::Parser::initialize_parse_rules();

    std::string prog = "3 + 4 ? 34 / 4 + 7 * 9 / 75 : 4 * 7 / 7 + 78";
    Scan::Scanner lexer(prog);

    Output output(prog);

    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse_precedence(-1);

    /* If there was a parse error, there will be some null pointers, so DO NOT
        actually compile. */
    if (output.had_error()) return -1;

    using namespace Instruction;
    Chunk chunk = Chunk();

    Compiler compiler(chunk);
    compiler.compile(node);

    chunk.print_code();
    delete node;

    return 0;
}