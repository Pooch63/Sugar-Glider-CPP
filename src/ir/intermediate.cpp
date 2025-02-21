#include "intermediate.hpp"
#include "../utils.hpp"

using namespace Intermediate;

const char* Intermediate::variable_type_to_string(VariableType type) {
    switch (type) {
        case VariableType::GLOBAL_CONSTANT: return "const var";
        case VariableType::GLOBAL_MUTABLE: return "mut var";
        case VariableType::FUNCTION_CONSTANT: return "func mut var";
        case VariableType::FUNCTION_MUTABLE: return "func const var";
        case VariableType::CLOSED_CONSTANT: return "closed const var";
        case VariableType::CLOSED_MUTABLE: return "closed mut var";
        case VariableType::NATIVE: return "native";
        default:
            throw sg_assert_error("Unknown variable type in string function");
    }
};

Variable::Variable(std::string *name, VariableType type, int scope, int function_ind) :
    name(name), type(type), scope(scope), function_ind(function_ind) {};
/* Updates type to express that it's closed. You might need to close global variables
    too if they are not part of the topmost scope. */
void Variable::close() {
    switch (this->type) {
        case GLOBAL_CONSTANT:
        case FUNCTION_CONSTANT:
            this->type = CLOSED_CONSTANT;
            break;
        case GLOBAL_MUTABLE:
        case FUNCTION_MUTABLE:
            this->type = CLOSED_MUTABLE;
            break;
        default: throw sg_assert_error("tried to close over invalid variable type"); break;
    }
}

// Constructor overloads >
Instruction::Instruction(InstrCode code) : code(code) {};
Instruction::Instruction(InstrCode code, std::string* payload) : code(code) {
    if (code == InstrCode::INSTR_STRING) {
        this->payload.str = payload;
    }
    else {
        this->payload.label = payload;
    }
};
Instruction::Instruction(InstrCode code, Operations::BinOpType type) :
    code(code), payload(ir_instruction_arg_t{ .bin_op = type }) {};
Instruction::Instruction(InstrCode code, Operations::UnaryOpType type) :
    code(code), payload(ir_instruction_arg_t{ .unary_op = type }) {};
Instruction::Instruction(InstrCode code, Variable *variable) :
    code(code), payload(ir_instruction_arg_t{ .variable = variable }) {};
Instruction::Instruction(InstrCode code, uint argument) : code(code) {
    if (code == InstrCode::INSTR_CALL) {
        this->payload.num_arguments = argument;
    }
    else if (code == InstrCode::INSTR_GET_FUNCTION_REFERENCE) {
        this->payload.function_index = argument;
    }
    else if (code == InstrCode::INSTR_MAKE_ARRAY) {
        this->payload.array_element_count = argument;
    }
    else {
        #ifdef DEBUG
        assert(false && "The instruction code given does not accept an unsigned integer (uint) argument");
        #endif
    }
}

Instruction::Instruction(Values::number_t number) :
    code(Intermediate::INSTR_NUMBER), payload(ir_instruction_arg_t{ .number = number }) {};
// < Constructor overloads

bool Instruction::is_truthy_constant() const {
    return this->code == InstrCode::INSTR_TRUE || this->code == InstrCode::INSTR_FALSE ||
        (this->has_number_payload() && this->get_number() != 0);
}
bool Instruction::is_constant() const {
    return this->code == InstrCode::INSTR_TRUE ||
        this->code == InstrCode::INSTR_FALSE ||
        this->code == InstrCode::INSTR_NULL ||
        this->code == InstrCode::INSTR_NUMBER ||
        this->code == InstrCode::INSTR_STRING ||
        this->code == InstrCode::INSTR_MAKE_ARRAY ||
        this->code == InstrCode::INSTR_GET_FUNCTION_REFERENCE;
}
bool Instruction::is_static_flow_load() const {
    if (this->code == InstrCode::INSTR_LOAD) return true;
    return this->is_constant();
}

Values::Value Instruction::payload_to_value() const {
    switch (this->code) {
        case InstrCode::INSTR_NUMBER:
            return Values::Value(Values::ValueType::NUMBER, this->get_number());
        case InstrCode::INSTR_TRUE:
            return Values::Value(Values::ValueType::TRUE);
        case InstrCode::INSTR_FALSE:
            return Values::Value(Values::ValueType::FALSE);
        case InstrCode::INSTR_NULL:
            return Values::Value(Values::ValueType::NULL_VALUE);
        case InstrCode::INSTR_STRING:
            return Values::Value(Values::ValueType::STRING, new std::string(*this->payload.str));
        case InstrCode::INSTR_GET_FUNCTION_REFERENCE:
            return Values::Value(this->get_function_index(), Values::ValueType::PROGRAM_FUNCTION);
        default:
            throw sg_assert_error("Tried to convert instruction payload to value that could not become a value");
    }
};

Instruction Instruction::value_to_instruction(Values::Value value) {
    switch (Values::get_value_type(value)) {
        case Values::ValueType::STRING:
            return Instruction(InstrCode::INSTR_STRING, Values::get_value_string(value));
        case Values::ValueType::NUMBER:
            return Instruction(Values::get_value_number(value));
        case Values::ValueType::TRUE:
            return Instruction(InstrCode::INSTR_TRUE);
        case Values::ValueType::FALSE:
            return Instruction(InstrCode::INSTR_FALSE);
        default:
            throw sg_assert_error("Tried to convert instruction to a value that could not become a value");
    }
};

void Instruction::free_payload() {
    if (this->code == InstrCode::INSTR_STRING) {
        delete this->payload.str;
    }
};

#include "../errors.hpp"

#include <cassert>
#include <iostream>

#include "../../lib/rang.hpp"

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
        case InstrCode::INSTR_STRING:
            return "INSTR_STRING";
        case InstrCode::INSTR_MAKE_ARRAY:
            return "INSTR_MAKE_ARRAY";
        case InstrCode::INSTR_GET_ARRAY_VALUE:
            return "INSTR_GET_ARRAY_VALUE";
        case InstrCode::INSTR_SET_ARRAY_VALUE:
            return "INSTR_SET_ARRAY_VALUE";
        case InstrCode::INSTR_GET_FUNCTION_REFERENCE:
            return "GET_FUNCTION_REFERENCE";
        case InstrCode::INSTR_MAKE_FUNCTION:
            return "MAKE_FUNCTION";
        case InstrCode::INSTR_RETURN:
            return "RETURN";
        case InstrCode::INSTR_LOAD:
            return "LOAD";
        case InstrCode::INSTR_STORE:
            return "STORE";
        case InstrCode::INSTR_CALL:
            return "CALL";
        case InstrCode::INSTR_EXIT:
            return "EXIT";
        default:
            throw sg_assert_error("Tried to convert unknown instruction type to string");
    }
};

rang::fg instruction_name_c = rang::fg::magenta;
rang::fg number_c = rang::fg::yellow;
rang::fg string_c = rang::fg::yellow;
rang::fg operation_c = rang::fg::cyan;
rang::fg label_c = rang::fg::blue;
rang::fg variable_c = rang::fg::red;
rang::fg comment_c = rang::fg::green;

/* Length of instruction name in the console */
static const uint INSTRUCTION_NAME_LENGTH = 30;
/* Space given to argument before logging the comment */
static const uint ARGUMENT_SPACE = IR_LABEL_LENGTH + 5;
/* Max length of a string in an argument */
static const uint MAX_STRING_LENGTH = 20;

// And max string length must be at least 3 to allow for "..."
static_assert(MAX_STRING_LENGTH >= 5);

void Intermediate::log_instruction(Instruction instr) {
    std::string type = Intermediate::instr_type_to_string(instr.code);
    
    std::cout << instruction_name_c << type << rang::style::reset;
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
            argument += *instr.payload.label;
            std::cout << label_c << rang::style::bold << argument;
        }
            break;
        case InstrCode::INSTR_BIN_OP:
        {
            Operations::BinOpType type = instr.payload.bin_op;
            argument = std::to_string(static_cast<uint>(type));
            argument += " (";
            argument += Operations::bin_op_to_string(type);
            argument += ')';
            
            std::cout << operation_c << argument;
        }
            break;
        case InstrCode::INSTR_UNARY_OP:
        {
            Operations::UnaryOpType type = instr.payload.unary_op;
            argument = std::to_string(static_cast<uint>(type));
            argument += " (";
            argument += Operations::unary_op_to_string(type);
            argument += ')';

            std::cout << operation_c << argument;
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
        case InstrCode::INSTR_STRING:
        {
            std::string* value = instr.payload.str;

            argument = '"';

            if (value->size() <= MAX_STRING_LENGTH) {
                argument += *value;
            }
            else {
                argument += value->substr(0, MAX_STRING_LENGTH - 3);
                argument += "...";
            }
            argument += '"';

            std::cout << string_c << argument;
            
            comment = "length=(";
            comment += std::to_string(value->size());
            comment += ")";
        }
            break;
        case InstrCode::INSTR_MAKE_ARRAY:
        {
            argument = std::to_string(instr.payload.array_element_count);
            std::cout << number_c << argument;

            comment = "[#elements=";
            comment += std::to_string(instr.payload.array_element_count);
            comment += "]";
        }
            break;
        case InstrCode::INSTR_GET_FUNCTION_REFERENCE:
        {
            std::cout << variable_c << instr.get_function_index();
        }
            break;
        case InstrCode::INSTR_LOAD:
        case InstrCode::INSTR_STORE:
        {
            std::string* name = instr.payload.variable->name;

            if (name->size() <= MAX_STRING_LENGTH) argument = *name;
            else {
                argument = name->substr(0, MAX_STRING_LENGTH - 3);
                argument += "...";
            }

            argument += var_ind_to_subscript(instr.payload.variable->scope);

            std::cout << variable_c << rang::style::underline << argument;
            
            comment = "(";
            comment += Intermediate::variable_type_to_string(instr.payload.variable->type);
            comment += ", function_scope = ";
            comment += std::to_string(instr.payload.variable->function_ind);
            comment += ")";
        }
            break;
        case InstrCode::INSTR_CALL:
        {
            argument = std::to_string(instr.get_argument_count());
            std::cout << number_c << argument;
        }
            break;
        default: break;
    }
    std::cout << rang::style::reset;

    argument_length = get_string_length_as_utf32(argument);

    if (comment.size() > 0) {
        if (argument.size() < ARGUMENT_SPACE) {
            for (uint space = 0; space < ARGUMENT_SPACE - argument_length; space += 1) {
                std::cout << ' ';
            }
        }
        std::cout << comment_c << "; " << comment;
        std::cout << rang::style::reset;
    }

    std::cout << '\n';
}
void Block::log_block() const {
    size_t size = 0;
    for (Label set : this->labels) size += set.instructions.size();

    uint code_count_length = get_digits(size);

    uint instr_number = 0;

    // Don't increment the loop, because print instruction always increases it by at least one
    for (uint set_count = 0; set_count < this->labels.size(); set_count += 1) {
        Label label = this->labels.at(set_count);
        std::cout << '\n';
        std::cout << label_c << rang::style::bold << ".L" << *label.name << ":\n";
        std::cout << rang::style::reset;

        for (Instruction instr : label.instructions) {
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
}

// Function >
Function::Function(Block *block) : block(std::unique_ptr<Block>(block)) {};
void Function::add_argument(Intermediate::Variable *argument) {
    this->arguments.push_back(argument);
}
// < Function

// Label IR >
LabelIR::LabelIR() : main(Function(new Block())) {}

Function *LabelIR::new_function() {
    Function *function = new Function(new Block());
    this->functions.push_back(function);
    return function;
}
int LabelIR::last_function_index() const {
    return this->functions.size() == 0 ? global_function_ind : static_cast<int>(this->functions.size()) - 1;
};
void LabelIR::log_ir() const {
    std::cout << "-------------------------------------------------------\n";
    std::cout << "                          IR\n";
    std::cout << rang::fg::green
              << "                       =>MAIN<=\n";
    std::cout << rang::style::reset;

    this->main.get_block()->log_block();

    for (uint func_ind = 0; func_ind < this->functions.size(); func_ind += 1) {
        std::cout << "-------------------------------------------------------\n";
        std::cout << '\n' << rang::fg::green;
        std::cout << "                       Function 0x" << std::hex << func_ind << std::dec << "\n";
        std::cout << rang::style::reset;

        this->functions[func_ind]->get_block()->log_block();
    }

    std::cout << "-------------------------------------------------------" << std::endl;
}
LabelIR::~LabelIR() {
    for (Function *function : this->functions) {
        delete function;
    }
}
// < Label IR

Intermediate::Label::Label(label_index_t* index) : name(index) {};

Label &Block::get_label_at_numerical_index(uint index) {
    return this->labels.at(index);
}

// Label name generation >
std::uniform_int_distribution<uint32_t> Block::label_generator =
                std::uniform_int_distribution<uint32_t>(0, CHAR_LABEL_COUNT);
static char label_chars[CHAR_LABEL_COUNT] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '-', '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};
label_index_t* Block::gen_label_name() const {
    label_index_t* label = new label_index_t("");
    for (uint i = 0; i < IR_LABEL_LENGTH; i += 1) {
        label->append(1, label_chars[Block::label_generator(Random::rng)]);
    }
    return label;
}
// < Label name generation

label_index_t* Block::new_label() {
    label_index_t* index = this->gen_label_name();
    this->new_label(index);
    return index;
}
void Block::new_label(label_index_t* index) {
    /* If there is no terminating goto command at the end, that is undefined.
        Add it automatically. */
    if (this->labels.size() > 0) {
        Label last = this->labels.back();
        // Add a GOTO if there's no terminating jump, or if it's an empty label.
        if (
            last.instructions.size() == 0 ||
            this->labels.back().instructions.back().code != InstrCode::INSTR_GOTO
        ) {
            this->labels.back().instructions.push_back(
                Intermediate::Instruction(
                    Intermediate::INSTR_GOTO,
                    index ) );
        }
    }

    this->labels.push_back(Label(index));
}
void Block::add_instruction(Intermediate::Instruction instruction) {
    if (this->labels.size() == 0) this->new_label();
    this->labels.back().instructions.push_back(instruction);
}

Block::~Block() {
    for (Label label : this->labels) {
        delete label.name;
    }
}