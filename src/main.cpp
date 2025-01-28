#include "compiler/ast.hpp"
#include "compiler/compiler.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "errors.hpp"
#include "ir/bytecode.hpp"
#include "ir/intermediate.hpp"
#include "ir/transpiler.hpp"
#include "optimizer/label-intermediate.hpp"
#include "utils.hpp"

#include <iostream>

int main() {
    Random::initialize_rng();

    std::string prog = "PI = 4; while (true) { var b = 4; var m = 3 * b; function l() { function z() {}; return m; } l(9); }";

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

    auto block = Intermediate::LabelIR();

    Compiler compiler(block, output);
    bool compile_success = compiler.compile(node);

    /* If there was a compiler error, don't continue. */
    if (!compile_success) {
        delete node;
        return Errors::COMPILE_ERROR;
    }

    block.log_ir();

    std::cout << "passed compilation" << std::endl;

    auto optimized = Intermediate::Block();
    optimize_labels(*block.get_main(), optimized);

    std::cout << "passed optimization" << std::endl;

    std::cout << std::endl;
    optimized.log_block();

    Bytecode::Chunk main =  Bytecode::Chunk();
    Runtime runtime = Runtime(main);

    Transpiler(runtime).transpile_label_to_bytecode(optimized);
    runtime.get_main()->print_code(&runtime);

    delete node;
    return 0;
}