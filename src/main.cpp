#include "ast.hpp"
#include "ir/bytecode.hpp"
#include "ir/intermediate.hpp"
#include "compiler.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>

int main() {
    Parse::Parser::initialize_parse_rules();

    std::string prog = "var g = 3; g;";
    Scan::Scanner lexer(prog);

    Output output(prog);

    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse_statement();

    std::cout << "DID PARSING WORK?" << std::endl;

    // block.add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, 1));
    // block.add_instruction(Intermediate::Instruction(Intermediate::INSTR_GOTO, 2));
    // block.log_block();

    /* If there was a parse error, there will be some null pointers, so DO NOT
        actually compile. */
    if (output.had_error()) {
        delete node;
        return -1;
    }

    auto block = Intermediate::Block();

    using namespace Bytecode;
    Chunk chunk = Chunk();

    // Compiler compiler(chunk);
    Compiler compiler(block);
    compiler.compile(node);


    // chunk.print_code();
    block.log_block();

    delete node;

    return 0;
}