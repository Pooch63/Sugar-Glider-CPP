#include "intermediate.hpp"
#include "../utils.hpp"

using namespace Intermediate;

Instruction::Instruction(InstrCode code) : code(code) {};
Instruction::Instruction(InstrCode code, address_t label) : code(code), payload(ir_instruction_arg_t{ .label = label }) {};
Instruction::Instruction(InstrCode code, Operations::BinOpType type) :
    code(code), payload(ir_instruction_arg_t{ .bin_op = type }) {};
Instruction::Instruction(InstrCode code, Operations::UnaryOpType type) :
    code(code), payload(ir_instruction_arg_t{ .unary_op = type }) {};
Instruction::Instruction(InstrCode code, Scopes::Variable variable) :
    code(code), payload(ir_instruction_arg_t{ .variable = variable }) {};

Instruction::Instruction(Values::number_t number) :
    code(Intermediate::INSTR_NUMBER), payload(ir_instruction_arg_t{ .number = number }) {};

Values::number_t Instruction::get_number() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_NUMBER);
    #endif
    return this->payload.number;
}
address_t Instruction::get_address() const {
    #ifdef DEBUG
    assert(this->is_jump());
    #endif
    return this->payload.label;
}
Operations::BinOpType Instruction::get_bin_op() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_BIN_OP);
    #endif
    return this->payload.bin_op;
}
Operations::UnaryOpType Instruction::get_unary_op() const {
    #ifdef DEBUG
    assert(this->code == InstrCode::INSTR_UNARY_OP);
    #endif
    return this->payload.unary_op;
}


bool Instruction::is_truthy_constant() const {
    return this->code == InstrCode::INSTR_TRUE || this->code == InstrCode::INSTR_FALSE ||
        (this->has_number_payload() && this->get_number() != 0);
}
bool Instruction::is_constant() const {
    return this->code == InstrCode::INSTR_TRUE ||
        this->code == InstrCode::INSTR_FALSE ||
        this->code == InstrCode::INSTR_NULL ||
        this->code == InstrCode::INSTR_NUMBER;
}

Values::Value Instruction::payload_to_value() const {
    switch (this->code) {
        case InstrCode::INSTR_NUMBER:
            return Values::Value(Values::ValueType::NUMBER, this->get_number());
        case InstrCode::INSTR_TRUE:
            return Values::Value(Values::ValueType::TRUE);
        case InstrCode::INSTR_FALSE:
            return Values::Value(Values::ValueType::FALSE);
        default:
            #ifdef DEBUG
            /* Can't convert this instruction to a value */
            assert(false);
            #endif
    }
};

Instruction Instruction::value_to_instruction(Values::Value value) {
    switch (value.get_type()) {
        case Values::ValueType::NUMBER:
            return Instruction(value.get_number());
        case Values::ValueType::TRUE:
            return Instruction(InstrCode::INSTR_TRUE);
        case Values::ValueType::FALSE:
            return Instruction(InstrCode::INSTR_FALSE);
        default:
            #ifdef DEBUG
            /* Can't convert this to an instruction */
            assert(false);
            #endif
    }
};

#ifdef DEBUG
#include "../errors.hpp"

#include <cassert>
#include <iostream>

using namespace Colors;
std::string instruction_name_c = create_color(Color::PURPLE);
std::string number_c = create_color(Color::YELLOW);
std::string label_c = create_color(Color::BLUE) + create_color(Color::BOLD);
std::string variable_c = create_color(Color::RED) + create_color(Color::UNDERLINE);
std::string comment_c = create_color(Color::GREEN);

const char* Intermediate::instr_type_to_string(InstrCode code) {
    switch (code) {
        case InstrCode::INSTR_POP:
            return "POP";
        case InstrCode::INSTR_GOTO:
            return "GOTO";
        case InstrCode::INSTR_POP_JIZ:
            return "POP_JIZ";
        case InstrCode::INSTR_POP_JNZ:
            return "POP_JNZ";
        case InstrCode::INSTR_BIN_OP:
            return "BIN_OP";
        case InstrCode::INSTR_UNARY_OP:
            return "UNARY_OP";
        case InstrCode::INSTR_TRUE:
            return "INSTR_TRUE";
        case InstrCode::INSTR_FALSE:
            return "INSTR_FALSE";
        case InstrCode::INSTR_NULL:
            return "INSTR_NULL";
        case InstrCode::INSTR_NUMBER:
            return "INSTR_NUMBER";
        case InstrCode::INSTR_LOAD:
            return "LOAD";
        case InstrCode::INSTR_STORE:
            return "STORE";
        case InstrCode::INSTR_EXIT:
            return "EXIT";
        default:
            #ifdef DEBUG
            assert(false);
            #endif
    }
};

/* Length of instruction name in the console */
static uint INSTRUCTION_NAME_LENGTH = 30;
/* Space given to argument before logging the comment */
static uint ARGUMENT_SPACE = 15;

static void log_instruction(Instruction instr) {
    std::string type = Intermediate::instr_type_to_string(instr.code);
    
    std::cout << instruction_name_c << type;
    set_color(Color::DEFAULT);
    for (uint space = type.size(); space < INSTRUCTION_NAME_LENGTH; space += 1) {
        std::cout << ' ';
    }

    /* The length of the argument, minus the ANSI code points */
    int argument_length = 0;
    /* The switch must log the argument, it's just that every single case needs this string to count the argument length. */
    std::string argument = "";
    std::string comment = "";

    /* The argument */
    switch (instr.code) {
        case InstrCode::INSTR_GOTO:
        case InstrCode::INSTR_POP_JIZ:
        case InstrCode::INSTR_POP_JNZ:
        {
            argument = ".L";
            argument += std::to_string(instr.payload.label);
            std::cout << label_c << argument;
            argument_length = argument.size();
        }
            break;
        case InstrCode::INSTR_BIN_OP:
        {
            Operations::BinOpType type = instr.payload.bin_op;
            argument = std::to_string(static_cast<uint>(type));
            argument += " (";
            argument += Operations::bin_op_to_string(type);
            argument += ')';
            
            std::cout << argument;
            argument_length = argument.size();
        }
            break;
        case InstrCode::INSTR_UNARY_OP:
        {
            Operations::UnaryOpType type = instr.payload.unary_op;
            argument = std::to_string(static_cast<uint>(type));
            argument += " (";
            argument += Operations::unary_op_to_string(type);
            argument += ')';

            std::cout << argument;
            argument_length = argument.size();
        }
            break;
        case InstrCode::INSTR_NUMBER:
        {
            argument = std::to_string(instr.payload.number);
            std::cout << number_c << argument;

            comment = "[type=";
            comment += typeid(Values::number_t).name();
            comment += "]";
        }
            break;
        case InstrCode::INSTR_LOAD:
        case InstrCode::INSTR_STORE:
        {
            argument = *instr.payload.variable.name;
            argument += number_as_subscript(instr.payload.variable.scope);

            std::cout << variable_c << argument;
            
            comment = "(";
            comment += (instr.payload.variable.type == Scopes::VariableType::CONSTANT ? "const" : "mut var");
            comment += ")";
        }
            break;
        default: break;
    }
    set_color(Colors::DEFAULT);

    argument_length = get_string_length_as_utf32(argument);

    if (comment.size() > 0) {
        if (argument.size() < ARGUMENT_SPACE) {
            for (uint space = 0; space < ARGUMENT_SPACE - argument_length; space += 1) {
                std::cout << ' ';
            }
        }
        std::cout << comment_c;
        std::cout << "; " << comment;
        set_color(Colors::DEFAULT);
    }

    std::cout << '\n';
}

Block::Block() {
}
void Block::log_block() const {
    size_t size = 0;
    for (intermediate_set_t set : this->labels) size += set.size();

    uint code_count_length = get_digits(size);

    uint instr_number = 0;

    std::cout << "-------------------------------------------------------\n";
    std::cout << "                          IR                           \n";
    // Don't increment the loop, because print instruction always increases it by at least one
    for (uint set_count = 0; set_count < this->labels.size(); set_count += 1) {
        std::cout << '\n';
        std::cout << label_c << ".L" << set_count << ":\n";
        set_color(Colors::DEFAULT);

        for (Instruction instr : this->labels.at(set_count)) {
            /* Log instruction number */
            uint index_length = get_digits(instr_number);
            for (uint space = 0; space < code_count_length - index_length; space += 1) {
                std::cout << ' ';
            }
            std::cout << "  " << std::to_string(instr_number) << " |  ";

            log_instruction(instr);
            instr_number += 1;
        }
    }
    
    std::cout << "-------------------------------------------------------" << std::endl;
}

#endif

intermediate_set_t &Block::get_label(uint index) { return this->labels.at(index); }


// Label name generation >
std::uniform_int_distribution<uint32_t> Block::label_generator =
                std::uniform_int_distribution<uint32_t>(0, CHAR_LABEL_COUNT);
static char label_chars[CHAR_LABEL_COUNT] = {
    '-', '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};
std::string Block::gen_label_name() const {
    std::string label = "";
    for (uint i = 0; i < 20; i += 1) {
        label += label_chars[Block::label_generator(Random::rng)];
    }
    return label;
}
std::string Block::new_label_name() const {
    std::string label = this->gen_label_name();

    return label;
}
// < Label name generation

void Block::new_label() {
    this->labels.push_back(intermediate_set_t());
}
void Block::add_instruction(Intermediate::Instruction instruction) {
    if (this->labels.size() == 0) this->new_label();
    this->labels.back().push_back(instruction);
}