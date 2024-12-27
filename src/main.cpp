#include "compiler/ast.hpp"
#include "compiler/compiler.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "errors.hpp"
#include "ir/bytecode.hpp"
#include "ir/intermediate.hpp"
#include "optimizer/label-intermediate.hpp"
#include "utils.hpp"

#include <iostream>

int main() {
    Parse::Parser::initialize_parse_rules();
    Random::initialize_rng();

    std::string prog = "var g = 1; if (g = 3) { g = g + 4; }";

    Output output(prog);
    Scan::Scanner lexer(prog, output);

    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse();

    std::cout << "DID PARSING WORK?" << std::endl;

    /* If there was a parse error, there will be some null pointers, so DO NOT
        actually compile. */
    if (output.had_error()) {
        delete node;
        return -1;
    }

    using Bytecode::Chunk;
    Chunk chunk = Chunk();
    auto block = Intermediate::Block();

    Compiler compiler(block, output);
    bool compile_success = compiler.compile(node);

    /* If there was a compiler error, don't continue. */
    if (!compile_success) {
        delete node;
        return -1;
    }

    auto optimized = Intermediate::Block();
    optimize_labels(block, optimized);

    block.log_block();
    std::cout << std::endl;
    optimized.log_block();

    delete node;
    return 0;
}