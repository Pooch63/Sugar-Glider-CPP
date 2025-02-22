#include "scopes.hpp"
#include "../globals.hpp"
#include "../natives/natives.hpp"

#ifdef DEBUG_ASSERT
#include <cassert>
#endif

using Intermediate::label_index_t, Intermediate::Variable, Intermediate::VariableType;
using namespace Scopes;

Scope::Scope(ScopeType type) : type(type) {
    // Make sure that this isn't a scope type that had to give more information
    #ifdef DEBUG
    assert(type == ScopeType::NORMAL || type == ScopeType::FUNCTION);
    #endif
};
Scope::Scope(ScopeType type, label_index_t* condition, label_index_t* end) :
    type(type), loop_condition(condition), loop_end(end) {
    // Make sure the type is a loop
    #ifdef DEBUG
    assert(type == ScopeType::LOOP);
    #endif
};

bool Scope::has_variable(std::string* name) const {
    auto variable = this->variables.find(*name);
    return variable != this->variables.end();
};
bool Scope::get_variable(std::string* name, Variable *&info) const {
    auto variable = this->variables.find(*name);
    // If no variable was found, return nullptr
    if (variable == this->variables.end()) return false;

    info = variable->second;
    return true;
};
Variable *Scope::add_variable(std::string* name, Variable *info) {
    auto insert = this->variables.emplace(*name, info);
    return insert.first->second;
};

label_index_t *Scope::get_loop_condition_label() const {
    #ifdef DEBUG_ASSERT
    assert(this->type == ScopeType::LOOP);
    #endif
    return this->loop_condition;
};
label_index_t* Scope::get_loop_end() const {
    #ifdef DEBUG_ASSERT
    assert(this->type == ScopeType::LOOP);
    #endif
    return this->loop_end;
};

ScopeManager::ScopeManager() {
    this->init_native_scope();
    this->scopes.push_back(native_scope);
}

bool ScopeManager::variable_exists(std::string* name) const {
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        if (this->scopes.at(index).has_variable(name)) return true;
    }
    return false;
}
bool ScopeManager::get_variable(std::string* name, Variable *&info) const {
    for (int index = this->scopes.size() - 1; index >= 0; index -= 1) {
        if (this->scopes.at(static_cast<uint>(index)).get_variable(name, info)) return true;
    }
    return false;
}
Variable *ScopeManager::add_variable(std::string* name, VariableType type, int function_index) {
    #ifdef DEBUG_ASSERT
    assert(this->scopes.size() > 0);
    #endif

    Variable *info = new Variable(
        new std::string(*name),
        type,
        this->get_function_scope(),
        function_index);
    this->variables.push_back(info);
    return this->scopes.back().add_variable(name, info);
}
bool ScopeManager::last_scope_has_variable(std::string* name) {
    return this->scopes.back().has_variable(name);
};

bool ScopeManager::in_function() const {
    for (int index = this->scopes.size() - 1; index >= 0; index -= 1) {
        if (this->scopes.at(static_cast<uint>(index)).get_type() == ScopeType::FUNCTION) return true;
    }
    return false;
};

void ScopeManager::new_scope(ScopeType type) {
    this->scopes.push_back(Scope(type));
}
void ScopeManager::new_scope(ScopeType type, label_index_t* condition, label_index_t* end) {
    this->scopes.push_back(Scope(type, condition, end));
};
void ScopeManager::pop_scope() {
    this->scopes.pop_back();
}

label_index_t* ScopeManager::get_loop_condition_label() const {
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        Scope scope = this->scopes.at(index);
        // Not a loop, but may be inside a loop
        if (scope.get_type() == ScopeType::NORMAL) continue;

        return scope.get_loop_condition_label();

    }
    return nullptr;
}
label_index_t* ScopeManager::get_loop_end() const {
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        Scope scope = this->scopes.at(index);
        // Not a loop, but may be inside a loop
        if (scope.get_type() == ScopeType::NORMAL) continue;

        return scope.get_loop_end();

    }
    return nullptr;
}
VariableType ScopeManager::add_variable_headers(VariableType basic_type) {
    if (!this->in_function()) return basic_type;

    switch (basic_type) {
        case VariableType::GLOBAL_CONSTANT: return VariableType::FUNCTION_CONSTANT;
        case VariableType::GLOBAL_MUTABLE:  return VariableType::FUNCTION_MUTABLE;
        default:
            throw sg_assert_error("Logic not yet implemented to add variable header to this variable type");
    }
};

ScopeManager::~ScopeManager() {
    for (Variable* variable : this->variables) {
        delete variable->name;
        delete variable;
    }
}

void ScopeManager::init_native_scope() {
    std::string* var_name;

    for (auto &name : Natives::name_to_native_index) {
        var_name = new std::string(name.first);

        Variable *variable = new Variable(var_name, VariableType::NATIVE, -1, Intermediate::global_function_ind);
        this->variables.push_back(variable);

        this->native_scope.add_variable(var_name, variable);
    }
}