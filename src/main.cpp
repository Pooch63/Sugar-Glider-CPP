#include "ast.hpp"
#include "bytecode.hpp"
#include "compiler.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>

int main() {
    std::string prog = "3 + -4 / 5";
    Scan::Scanner lexer(prog);

    // std::cout << "was NOT the seg fault!" << std::endl;

    // std::cout << lexer.next_token().to_string() << std::endl;
    // std::cout << lexer.next_token().get_type() << std::endl;
    // std::cout << lexer.next_token().to_string() << std::endl;

    Output output(prog);
    output.error(TokenPosition{ .line = 0, .col = 2, .length = 1 }, "This is a test error");


    Parse::Parser::initialize_parse_rules();

    Parse::Parser parser(lexer, output);
    AST::Node* node = parser.parse_precedence(-1);

    std::cout << node->get_type() << std::endl;

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