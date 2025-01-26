#include "transpiler.hpp"

using Intermediate::Instruction, Intermediate::InstrCode, Intermediate::Label, Intermediate::label_index_t, Bytecode::address_t;

Transpiler::Transpiler(Runtime &runtime) : runtime(runtime), chunk(runtime.get_main()) {};

void Transpiler::transpile_ir_instruction(Instruction instr) {
    switch (instr.code) {
        // 0 argument instructions
        case InstrCode::INSTR_POP: chunk->push_opcode(OpCode::OP_POP); break;
        case InstrCode::INSTR_RETURN: chunk->push_opcode(OpCode::OP_RETURN); break;
        case InstrCode::INSTR_EXIT: chunk->push_opcode(OpCode::OP_EXIT); break;

        // 1 argument operations
        case InstrCode::INSTR_BIN_OP:
            chunk->push_opcode(OpCode::OP_BIN);
            chunk->push_bin_op_type(instr.get_bin_op());
            break;
        case InstrCode::INSTR_UNARY_OP:
            chunk->push_opcode(OpCode::OP_UNARY);
            chunk->push_unary_op_type(instr.get_unary_op());
            break;
        case InstrCode::INSTR_CALL:
            chunk->push_opcode(OpCode::OP_CALL);
            chunk->push_value<Bytecode::call_arguments_t>(instr.get_argument_count());
            break;

        // Push constant instructors
        case InstrCode::INSTR_NUMBER:
        case InstrCode::INSTR_STRING:
            chunk->push_opcode(OpCode::OP_LOAD_CONST);
            chunk->push_value<Bytecode::constant_index_t>(this->runtime.new_constant(instr.payload_to_value()));
            break;
        default: break;
    }
}

void Transpiler::transpile_label_to_bytecode(Intermediate::Block& labels) {
    address_t byte_count = 0;
    for (Label label : labels) {
        label_starts[label.name] = byte_count;
        for (Instruction instr : label.instructions) {
            this->transpile_ir_instruction(instr);
        }
    }
};
