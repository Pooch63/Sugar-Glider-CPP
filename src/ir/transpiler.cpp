#include "transpiler.hpp"
#include "../runtime/natives.hpp"

using Intermediate::Instruction, Intermediate::InstrCode, Intermediate::Label, Intermediate::label_index_t, Bytecode::address_t;

Jump_Argument::Jump_Argument(Bytecode::address_t byte_address, Intermediate::label_index_t *label) :
    byte_address(byte_address), label(label) {};

func_var_info_t::func_var_info_t(Intermediate::Function *func) : func(func) {}

Transpiler::Transpiler(Runtime &runtime) : runtime(runtime) {};

void Transpiler::transpile_variable_instruction(Instruction instr, Intermediate::Function *func) {
    Intermediate::Variable variable = *instr.get_variable();

    if (variable.is_global()) {
        Bytecode::variable_index_t index;
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
            chunk->push_opcode(OpCode::OP_LOAD_GLOBAL);
        }
        else {
            chunk->push_opcode(OpCode::OP_STORE_GLOBAL);
        }
        chunk->push_value<Bytecode::variable_index_t>(index);
    }
    else if (variable.is_local_function_var()) {
        #ifdef DEBUG
        assert(func != nullptr && "Tried to compile function var that wasn't in a function");
        #endif

        Bytecode::variable_index_t index;
        bool is_arg = false;
        func_var_info_t &info = this->func_variables.at(variable.function_ind);

        // First check to see if it's an argument
        for (uint arg_ind = 0; arg_ind < info.func->argument_count(); arg_ind += 1) {
            Intermediate::Variable *arg = info.func->get_argument(arg_ind);
            if (*arg == variable) {
                index = arg_ind;
                is_arg = true;
            }
        }

        if (!is_arg) {
            auto index_pair = info.hash.find(variable);
            if (index_pair != info.hash.end()) {
                index = index_pair->second;
            }
            else {
                index = info.hash.size();
                // Add it to the hashmap
                info.hash.emplace(variable, index);
            }
        }

        if (instr.code == InstrCode::INSTR_LOAD) {
            chunk->push_opcode(OpCode::OP_LOAD_FRAME_VAR);
        }
        else {
            chunk->push_opcode(OpCode::OP_STORE_FRAME_VAR);
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
void Transpiler::transpile_ir_instruction(Instruction instr, Intermediate::Function *func) {
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

        // Push constant instructions
        case InstrCode::INSTR_TRUE: chunk->push_opcode(OpCode::OP_TRUE); break;
        case InstrCode::INSTR_FALSE: chunk->push_opcode(OpCode::OP_FALSE); break;
        case InstrCode::INSTR_NULL: chunk->push_opcode(OpCode::OP_NULL); break;
        case InstrCode::INSTR_NUMBER:
        case InstrCode::INSTR_STRING:
        case InstrCode::INSTR_GET_FUNCTION_REFERENCE:
            chunk->push_opcode(OpCode::OP_LOAD_CONST);
            chunk->push_value<Bytecode::constant_index_t>(this->runtime.new_constant(instr.payload_to_value()));
            instr.free_payload();
            break;

        case InstrCode::INSTR_GET_ARRAY_VALUE:
            chunk->push_opcode(OpCode::OP_GET_ARRAY_VALUE);
            break;
        case InstrCode::INSTR_SET_ARRAY_VALUE:
            chunk->push_opcode(OpCode::OP_SET_ARRAY_VALUE);
            break;
        case InstrCode::INSTR_MAKE_ARRAY:
            chunk->push_opcode(OpCode::OP_MAKE_ARRAY);
            chunk->push_value<Bytecode::variable_index_t>(instr.get_array_element_count());
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
            this->transpile_variable_instruction(instr, func);
        }
            break;

        case InstrCode::INSTR_MAKE_FUNCTION:
        {}
            break;
    }
}

void Transpiler::transpile_single_block(Intermediate::Function *func) {
    Intermediate::Block *labels = func->get_block();
    for (size_t label_ind = 0; label_ind < labels->label_count(); label_ind += 1) {
        Label label = labels->get_label_at_numerical_index(label_ind);
        label_starts[*label.name] = chunk->code_byte_count();
        for (size_t instr_ind = 0; instr_ind < label.instructions.size(); instr_ind += 1) {
            Instruction instr = label.instructions.at(instr_ind);

            // If it's a goto at the very end of a label that goes to the next label, don't include it.
            // This was necessary for the IR but redundant
            if (
                // Make sure this is not the last label
                label_ind < labels->label_count() - 1 &&
                // Make sure this is the last instruction in the label
                instr_ind == label.instructions.size() - 1 &&
                // Make sure it's a goto instruction
                instr.code == InstrCode::INSTR_GOTO &&
                // Make sure it goes to the next label
                *instr.get_address() == *labels->get_label_at_numerical_index(label_ind + 1).name
            ) {
                break;
            }

            this->transpile_ir_instruction(instr, func);
        }
    }

    // Insert jump arguments now that we know the positions of labels
    for (const Jump_Argument &jump : this->jump_arguments) {
        Bytecode::address_t address = this->label_starts[*jump.label];
        chunk->insert_address(jump.byte_address, address);
    }

    /* Clear label starts info, since it's only specific to this label.
        But make sure not to clear variable info */
    this->label_starts.clear();
};
void Transpiler::transpile_ir_to_bytecode(Intermediate::LabelIR &ir) {
    this->chunk = runtime.get_main();
    this->transpile_single_block(ir.get_main());

    for (int func_ind = 0; func_ind <= ir.last_function_index(); func_ind += 1) {
        auto chunk = Bytecode::Chunk();
        this->chunk = &chunk;
        Intermediate::Function *func = ir.get_function(func_ind);

        // Add new variable to index hashmap
        this->func_variables.push_back(func_var_info_t(func));

        this->transpile_single_block(func);

        Bytecode::call_arguments_t num_arguments = static_cast<Bytecode::call_arguments_t>(func->argument_count());
        Bytecode::variable_index_t total_variables = num_arguments + this->func_variables.back().func->argument_count();

        RuntimeFunction runtime_func = RuntimeFunction(chunk, num_arguments, total_variables);
        this->runtime.add_function(runtime_func);
    }
}