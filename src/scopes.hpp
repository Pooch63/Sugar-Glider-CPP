#ifndef _SGCPP_SCOPES_HPP
#define _SGCPP_SCOPES_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace Scopes {
    enum VariableType {
        CONSTANT,
        MUTABLE
    };

    struct Variable {
        std::string* name;
        VariableType type;
        int scope;
    };
    class Scope {
        private:
            std::unordered_map<std::string, Variable> variables = std::unordered_map<std::string, Variable>();

        public:
            bool has_variable(std::string* name) const;
            /* Returns false if there was no instance, otherwise returns true and updates the variable info. */
            bool get_variable(std::string* name, Variable &info) const;
            void add_variable(std::string* name, Variable info);
    };

    class ScopeManager {
        private:
            std::vector<Scope> scopes = std::vector<Scope>();

        public:
            bool variable_exists(std::string* name) const;
            /* Returns false if there was no instance. Otherwise, returns true and updates the variable info.
                Goes up through the scope tree to try to find the variable. */
            bool get_variable(std::string* name, Variable &info) const;
            /* Add a variable to the current scope. */
            Variable add_variable(std::string* name, VariableType type);

            bool last_scope_has_variable(std::string* name);

            void new_scope();
            void pop_scope();
    };
}

#endif