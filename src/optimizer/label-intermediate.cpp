#include "label-intermediate.hpp"

#include <unordered_map>

using Intermediate::intermediate_set_t, Intermediate::Instruction, Intermediate::InstrCode;

void optimize_labels(Intermediate::Block &old, Intermediate::Block &optimized) {
    using Intermediate::Label, Intermediate::label_index_t;

    /* Maximum size of a label to be unrolled */
    const int MAX_UNROLL_SIZE = 1;

    // Store the labels, then at the end, transfer them to the optimized
    std::vector<Label> labels = std::vector<Label>();

    /* Constant folding optimizer. */
    for (uint label_ind = 0; label_ind < old.label_count(); label_ind += 1) {
        Label &old_label = old.get_label_at_numerical_index(label_ind);
        labels.push_back(Label(old_label.name));
        intermediate_set_t &label = labels.back().instructions;

        for (uint index = 0; index < old_label.instructions.size(); index += 1) {
            Instruction instr = old_label.instructions.at(index);
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

                last.free_payload();

                continue;
            }
            /* Binop folding */
            if (label.size() >= 2 && label.at(label.size() - 2).is_constant() && last.is_constant() && instr.code == InstrCode::INSTR_BIN_OP) {
                Values::Value a = label.at(label.size() - 2).payload_to_value();
                Values::Value b = last.payload_to_value();
                
                Values::Value* result = Values::bin_op(instr.get_bin_op(), a, b);

                if (result == nullptr) {
                    // Just push the binop
                    label.push_back(instr);

                    // Make sure the faulty instructions' payloads are still valid
                    a.mark_payload();
                    b.mark_payload();

                    delete result;
                    continue;
                }

                result->mark_payload();

                // Pop the last two loads
                label.pop_back();
                label.pop_back();
                // Push the result
                label.push_back(Instruction::value_to_instruction(*result));

                label.at(label.size() - 2).free_payload();
                last.free_payload();

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
            if (last.is_static_flow_load() && instr.code == InstrCode::INSTR_POP) {
                last.free_payload();
                label.pop_back();
                continue;
            }
            /* DCE after return statement */
            if (last.code == InstrCode::INSTR_RETURN) {
                break;
            }

            label.push_back(instr);
        }
    }

    /* DCE: Remove unreachable code within labels */
    std::vector<Label> reachable = std::vector<Label>();
    for (Label label : labels) {
        reachable.push_back(Label(label.name));

        for (Instruction instr : label.instructions) {
            reachable.back().instructions.push_back(instr);
            if (instr.code == InstrCode::INSTR_GOTO) break;
        }
    }
    labels = reachable;

    /* Label unrolling */

    // This lookup table's keys are the labels that can be unrolled
    std::unordered_map<label_index_t, Label> small = std::unordered_map<label_index_t, Label>();

    // Get a list of small labels
    for (Label label : labels) {
        if (label.instructions.size() <= MAX_UNROLL_SIZE) {
            small.insert({ *label.name, label });
        }
    }
    // Now replace those small labels in every instance
    std::vector<Label> unrolled = std::vector<Label>();
    for (Label label : labels) {
        unrolled.push_back(Label(label.name));
        for (Instruction instr : label.instructions) {
            bool unroll = instr.code == InstrCode::INSTR_GOTO;

            if (!unroll) {
                unrolled.back().instructions.push_back(instr);
                continue;
            }

            auto found = small.find(*instr.get_address());

            if (found == small.end()) {
                unrolled.back().instructions.push_back(instr);
                continue;
            }

            for (Instruction replaced : found->second.instructions) {
                unrolled.back().instructions.push_back(replaced);
            }
        }
    }
    labels = unrolled;

    /* Dead code elimination */

    /* REMOVE all labels that are literally never used. */
    // This lookup table stores the number of jumps to a label. If a label is not a key in
    // this map, it means that it was not referenced.
    // We will remove any label that is not referenced
    std::unordered_map<label_index_t, uint> jumped = std::unordered_map<label_index_t, uint>();

    // Gather list of labels that are actually used.
    // Add first label, since that's the entry point
    for (uint label_ind = 0; label_ind < labels.size(); label_ind += 1) {
        Label label = labels[label_ind];
        // Set to 1 so that even if nothing references this label, we still keep it
        if (label_ind == 0) jumped[*label.name] = 1;

        for (Instruction instr : label.instructions) {
            if (!instr.is_jump()) continue;
    
            auto reference_object = jumped.find(*instr.get_address());
            // // The number of times the label was referenced
            uint references = 0;
            if (reference_object != jumped.end()) references = reference_object->second;

            jumped[*instr.get_address()] = references + 1;
        }
    }
    // Now, remove the unnused blocks
    std::vector<Label> dce = std::vector<Label>();
    for (Label label : labels) {
        auto jumped_label = jumped.find(*label.name);
        // Make sure the label was referenced and at least 1 instruction still needs it.
        if (jumped_label != jumped.end() && jumped_label->second > 0) dce.push_back(label);
        // If we delete the label, then remove all the references that the
        // label contained. This prevents the need for multiple passes
        // to remove all unused labels.
        else {
            for (Instruction instr : label.instructions) {
                if (!instr.is_jump()) continue;

                // This time, we know that the label the jump referenced is already in the map,
                // because it's part of a jump and must have been added.
                jumped[*instr.get_address()] = jumped[*instr.get_address()] - 1;
            }
        }
    }
    labels = dce;

    /* Transfer instructions */
    for (Label label : labels) {
        optimized.new_label(new std::string(*label.name));
        for (Instruction instr : label.instructions) {
            optimized.add_instruction(instr);
        }
    }
}