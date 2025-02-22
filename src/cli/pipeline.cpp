#include "../compiler/ast.hpp"
#include "../compiler/compiler.hpp"
#include "../compiler/lexer.hpp"
#include "../compiler/parser.hpp"
#include "../ir/transpiler.hpp"
#include "../optimizer/label-intermediate.hpp"
#include "../runtime/runtime.hpp"

#include "pipeline.hpp"

static int get_bytecode(std::string prog, Runtime &runtime) {
    Output output(prog);
    Scan::Scanner lexer(prog, output);
    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse();

    /* If there was a parse error, there will be some null pointers, so DO NOT
        actually compile. */
    if (output.had_error()) {
        delete node;
        return output.get_error();
    }

    auto block = Intermediate::LabelIR();

    Compiler compiler(block, output);
    bool compile_success = compiler.compile(node);

    delete node;

    std::cout << "got past compilaton\n";

    /* If there was a compiler error, don't continue. */
    if (!compile_success) {
        return Errors::COMPILE_ERROR;
    }

    block.log_ir();

    auto optimized = Intermediate::LabelIR();
    optimize_labels(block, optimized);

    std::cout << "got past optimization\n";

    optimized.log_ir();

    Transpiler transpiler = Transpiler(runtime);
    // Use optimized bytecode for program
    transpiler.transpile_ir_to_bytecode(optimized);

    runtime.init_global_pool(transpiler.num_variable_slots());

    runtime.log_instructions();

    return 0;
}

int run_file(std::string prog) {
    Bytecode::Chunk main = Bytecode::Chunk();
    Runtime runtime = Runtime(main);

    int compile_code = get_bytecode(prog, runtime);
    if (compile_code != 0) return compile_code;

    int code = runtime.run();
    return code;
}