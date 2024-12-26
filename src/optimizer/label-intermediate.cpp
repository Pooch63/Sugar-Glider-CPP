#include "label-intermediate.hpp"

using Intermediate::intermediate_set_t, Intermediate::Instruction, Intermediate::InstrCode;

void optimize_labels(Intermediate::Block &old, Intermediate::Block &optimized) {
    // Store the labels, then at the end, transfer them to the optimized
    std::vector<intermediate_set_t> labels = std::vector<intermediate_set_t>();

    /* Constant folding optimizer */
    for (uint label_ind = 0; label_ind < old.label_count(); label_ind += 1) {
        labels.push_back(intermediate_set_t());
        intermediate_set_t &label = labels.back();
        intermediate_set_t &old_label = old.get_label(label_ind);

        for (uint index = 0; index < old_label.size(); index += 1) {
            Instruction instr = old_label.at(index);
            if (label.size() == 0) {
                label.push_back(instr);
                continue;
            }

            // The last instruction in the optimized set
            Instruction last = label.at(label.size() - 1);

            /* Jump folding */
            if (last.is_constant() && (instr.code == InstrCode::INSTR_POP_JIZ || instr.code == InstrCode::INSTR_POP_JNZ)) {
                // Remove the constant load, since we'll pop it anyway.
                label.pop_back();

                bool jump_needs_truth = instr.code == InstrCode::INSTR_POP_JNZ;

                /* Add the goto if the constant was falsey AND it was a JIZ, OR, the constant was truthy AND it was a JNZ */
                if (!(last.is_truthy_constant() ^ jump_needs_truth)) {
                    label.push_back(
                        Instruction(InstrCode::INSTR_GOTO, instr.get_address())
                    );
                }

                continue;
            }
            /* Binop folding */
            if (label.size() >= 2 && label.at(label.size() - 2).is_constant() && last.is_constant() && instr.code == InstrCode::INSTR_BIN_OP) {
                Values::Value* result = Values::bin_op(instr.get_bin_op(), label.at(label.size() - 2).payload_to_value(), last.payload_to_value());

                if (result == nullptr) {
                    // Just push the binop
                    label.push_back(instr);
                    delete result;
                    continue;
                }

                // Pop the last two loads
                label.pop_back();
                label.pop_back();
                // Push the result
                label.push_back(Instruction::value_to_instruction(*result));

                delete result;
                continue;
            }
            /* Unary op folding */
            if (last.is_constant() && instr.code == InstrCode::INSTR_UNARY_OP) {
                Values::Value* result = Values::unary_op(instr.get_unary_op(), last.payload_to_value());

                if (result == nullptr) {
                    // Just push the unary op
                    label.push_back(instr);
                    delete result;
                    continue;
                }

                // Pop the argument load
                label.pop_back();
                // Push the result
                label.push_back(Instruction::value_to_instruction(*result));

                delete result;
                continue;
            }
            /* Unnecessary constant folding */
            if (last.is_constant() && instr.code == InstrCode::INSTR_POP) {
                label.pop_back();
                continue;
            }

            label.push_back(instr);
        }
    }

    /* Transfer instructions */
    for (intermediate_set_t label : labels) {
        optimized.new_label();
        for (Instruction instr : label) {
            optimized.add_instruction(instr);
        }
    }
}