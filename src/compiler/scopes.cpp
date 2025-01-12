#include "scopes.hpp"
#include "../globals.hpp"

#ifdef DEBUG_ASSERT
#include <cassert>
#endif

using Intermediate::label_index_t, Intermediate::Variable, Intermediate::VariableType;
using namespace Scopes;

Scope::Scope(ScopeType type) : type(type) {
    // Make sure that this isn't a scope type that had to give more information
    #ifdef DEBUG
    assert(type == ScopeType::NORMAL);
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
bool Scope::get_variable(std::string* name, Variable &info) const {
    auto variable = this->variables.find(*name);
    // If no variable was found, return nullptr
    if (variable == this->variables.end()) return false;

    info = variable->second;
    return true;
};
void Scope::add_variable(std::string* name, Variable info) {
    this->variables[*name] = info;
};

label_index_t * Scope::get_loop_condition_label() const {
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
    if (!ScopeManager::native_scope_initialized) ScopeManager::init_native_scope();

    this->scopes.push_back(native_scope);
}

bool ScopeManager::variable_exists(std::string* name) const {
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        if (this->scopes.at(index).has_variable(name)) return true;
    }
    return false;
}
bool ScopeManager::get_variable(std::string* name, Variable &info) const {
    for (int index = this->scopes.size() - 1; index >= 0; index -= 1) {
        if (this->scopes.at(static_cast<uint>(index)).get_variable(name, info)) return true;
    }
    return false;
}
Variable ScopeManager::add_variable(std::string* name, VariableType type) {
    #ifdef DEBUG_ASSERT
    assert(this->scopes.size() > 0);
    #endif

    Variable info = Variable{ .name = name, .type = type, .scope = static_cast<int>(this->scopes.size()) - 1 };
    this->scopes.back().add_variable(name, info);
    return info;
}
bool ScopeManager::last_scope_has_variable(std::string* name) {
    return this->scopes.back().has_variable(name);
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

bool ScopeManager::native_scope_initialized = false;

Scope Scopes::native_scope = Scope(ScopeType::NORMAL);

void ScopeManager::init_native_scope() {
    if (ScopeManager::native_scope_initialized) return;
    ScopeManager::native_scope_initialized = true;

    constexpr int var_count = 1;
    const char* variables[var_count] = {
        "PI"
    };

    std::string* var;

    for (int ind = 0; ind < var_count; ind += 1) {
        var = new std::string(variables[ind]);
        native_scope.add_variable(var, Variable{ .name = var, .type = VariableType::CONSTANT, .scope = -1 });
    }
}