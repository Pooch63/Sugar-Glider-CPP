#include "ast.hpp"
#include "bytecode.hpp"
#include "compiler.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>

int main() {
    std::string prog = "(4 / 5)";
    Scan::Scanner lexer(prog);

    // std::cout << "was NOT the seg fault!" << std::endl;

    // std::cout << lexer.next_token().to_string() << std::endl;
    // std::cout << lexer.next_token().get_type() << std::endl;
    // std::cout << lexer.next_token().to_string() << std::endl;

    Output output(prog);

    Parse::Parser::initialize_parse_rules();

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

    // chunk.push_small_enum<OpCode>(OpCode::OP_NUMBER);
    // chunk.push_number_value(1.58);
    // chunk.push_small_enum<OpCode>(OpCode::OP_TRUE);
    // chunk.push_small_enum<OpCode>(OpCode::OP_BIN);
    // chunk.push_small_enum<Operations::BinOpType>(Operations::BinOpType::BINOP_ADD);

    // chunk.print_code();

    delete node;

    return 0;
}