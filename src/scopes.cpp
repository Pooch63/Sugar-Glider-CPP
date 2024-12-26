#include "scopes.hpp"

using namespace Scopes;

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

bool ScopeManager::variable_exists(std::string* name) const {
    printf("Inside variable exists\n");
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        if (this->scopes.at(index).has_variable(name)) return true;
    }
    return false;
}
bool ScopeManager::get_variable(std::string* name, Variable &info) const {
    for (uint index = this->scopes.size() - 1; index > 0; index -= 1) {
        if (this->scopes.at(index).get_variable(name, info)) return true;
    }
    return false;
}
Variable ScopeManager::add_variable(std::string* name) {
    Variable info = Variable{ .name = name, .scope = static_cast<int>(this->scopes.size()) - 1 };
    this->scopes.back().add_variable(name, info);
    return info;
}
bool ScopeManager::last_scope_has_variable(std::string* name) {
    return this->scopes.back().has_variable(name);
};

void ScopeManager::new_scope() {
    this->scopes.push_back(Scope());
}
void ScopeManager::pop_scope() {
    this->scopes.pop_back();
}