#include "runtime.hpp"

Runtime::Runtime(Bytecode::Chunk &main) : main(main) {};

Bytecode::variable_index_t Runtime::new_constant(Values::Value value) {
    this->constants.push_back(value);
    return this->constants.size() - 1;
}