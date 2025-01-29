#include "transpiler.hpp"
#include "../runtime/natives.hpp"

using Intermediate::Instruction, Intermediate::InstrCode, Intermediate::Label, Intermediate::label_index_t, Bytecode::address_t;

Jump_Argument::Jump_Argument(Bytecode::address_t byte_address, Intermediate::label_index_t *label) :
    byte_address(byte_address), label(label) {};

Transpiler::Transpiler(Runtime &runtime) : runtime(runtime), chunk(runtime.get_main()) {};

void Transpiler::transpile_variable_instruction(Instruction instr) {
    Bytecode::variable_index_t index;
    Intermediate::Variable variable = *instr.get_variable();

    if (variable.type == Intermediate::GLOBAL_CONSTANT || variable.type == Intermediate::GLOBAL_MUTABLE) {
        auto index_pair = this->variables.find(variable);
        if (index_pair != this->variables.end()) {
            index = index_pair->second;
        }
        else {
            index = this->variables.size();
            // Add it to the hashmap
            this->variables.emplace(variable, index);
        }

        if (instr.code == InstrCode::INSTR_LOAD) {
            chunk->push_opcode(OpCode::OP_LOAD);
        }
        else {
            chunk->push_opcode(OpCode::OP_STORE);
        }
        chunk->push_value<Bytecode::variable_index_t>(index);
    }
    else if (variable.type == Intermediate::NATIVE) {
        chunk->push_opcode(OpCode::OP_LOAD_NATIVE);
        chunk->push_value<Bytecode::variable_index_t>(
            static_cast<Bytecode::variable_index_t>(Natives::get_native_index(*variable.name))
        );
    }
}
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
        case InstrCode::INSTR_GET_FUNCTION_REFERENCE:
            chunk->push_opcode(OpCode::OP_GET_FUNCTION_REFERENCE);
            chunk->push_value<Bytecode::variable_index_t>(static_cast<Bytecode::variable_index_t>(instr.get_function_index()));
            break;

        // Push constant instructions
        case InstrCode::INSTR_TRUE: chunk->push_opcode(OpCode::OP_TRUE); break;
        case InstrCode::INSTR_FALSE: chunk->push_opcode(OpCode::OP_FALSE); break;
        case InstrCode::INSTR_NULL: chunk->push_opcode(OpCode::OP_NULL); break;
        case InstrCode::INSTR_NUMBER:
        case InstrCode::INSTR_STRING:
            chunk->push_opcode(OpCode::OP_LOAD_CONST);
            chunk->push_value<Bytecode::constant_index_t>(this->runtime.new_constant(instr.payload_to_value()));
            instr.free_payload();
            break;

        case InstrCode::INSTR_GOTO:
        case InstrCode::INSTR_POP_JIZ:
        case InstrCode::INSTR_POP_JNZ:
        {
            switch (instr.code) {
                case InstrCode::INSTR_GOTO: chunk->push_opcode(Bytecode::OP_GOTO); break;
                case InstrCode::INSTR_POP_JIZ: chunk->push_opcode(Bytecode::OP_POP_JIZ); break;
                case InstrCode::INSTR_POP_JNZ: chunk->push_opcode(Bytecode::OP_POP_JNZ); break;
                default: break;
            }
            this->jump_arguments.push_back(Jump_Argument(static_cast<Bytecode::address_t>(chunk->code_byte_count()), instr.get_address()));
            // Push garbage until we update it later
            chunk->push_address(38104);
        }
            break;

        case InstrCode::INSTR_STORE:
        case InstrCode::INSTR_LOAD:
        {
            this->transpile_variable_instruction(instr);
        }
            break;

        case InstrCode::INSTR_MAKE_FUNCTION:
        {}
            break;
    }
}

void Transpiler::transpile_label_to_bytecode(Intermediate::Block& labels) {
    for (size_t label_ind = 0; label_ind < labels.label_count(); label_ind += 1) {
        Label label = labels.get_label_at_numerical_index(label_ind);
        label_starts[*label.name] = chunk->code_byte_count();
        for (size_t instr_ind = 0; instr_ind < label.instructions.size(); instr_ind += 1) {
            Instruction instr = label.instructions.at(instr_ind);

            // If it's a goto at the very end of a label that goes to the next label, don't include it.
            // This was necessary for the IR but redundant
            if (
                // Make sure this is not the last label
                label_ind < labels.label_count() - 1 &&
                // Make sure this is the last instruction in the label
                instr_ind == label.instructions.size() - 1 &&
                // Make sure it's a goto instruction
                instr.code == InstrCode::INSTR_GOTO &&
                // Make sure it goes to the next label
                *instr.get_address() == *labels.get_label_at_numerical_index(instr_ind + 1).name
            ) {
                break;
            }

            this->transpile_ir_instruction(instr);
        }
    }

    // Insert jump arguments now that we know the positions of labels
    for (const Jump_Argument &jump : this->jump_arguments) {
        Bytecode::address_t address = this->label_starts[*jump.label];(void)address;
        chunk->insert_address(jump.byte_address, address);
    }
};
