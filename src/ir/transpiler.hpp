#ifndef _SGCPP_TRANSPILER_HPP
#define _SGCPP_TRANSPILER_HPP

#include "bytecode.hpp"
#include "intermediate.hpp"

/* Transpile label intermediate to bytecode */
void transpile_label_to_bytecode(Intermediate::Block& labels, Bytecode::Chunk& chunk);

#endif