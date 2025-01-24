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
    Random::initialize_rng();

    std::string prog = "function _(_) { return _; _ = 5; }; _ = 4; \"\\U00000065\"; ((((((_))))))(3);";

    Output output(prog);
    Scan::Scanner lexer(prog, output);

    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse();

    std::cout << "DID PARSING WORK?" << std::endl;

    /* If there was a parse error, there will be some null pointers, so DO NOT
        actually compile. */
    if (output.had_error()) {
        delete node;
        return output.get_error();
    }

    using Bytecode::Chunk;
    Chunk chunk = Chunk();
    auto block = Intermediate::LabelIR();

printf("?maybe?\n");
    Compiler compiler(block, output);
    bool compile_success = compiler.compile(node);
printf("!yes!\n");

    /* If there was a compiler error, don't continue. */
    if (!compile_success) {
        delete node;
        return Errors::COMPILE_ERROR;
    }

    block.log_ir();

    auto optimized = Intermediate::Block();
    optimize_labels(*block.get_main(), optimized);

    std::cout << std::endl;
    optimized.log_block();

    delete node;
    return 0;
}