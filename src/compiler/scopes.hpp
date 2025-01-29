#ifndef _SGCPP_SCOPES_HPP
#define _SGCPP_SCOPES_HPP

#include "../ir/intermediate.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace Scopes {
    enum ScopeType {
        NORMAL,
        LOOP,
        FUNCTION
    };

    class Scope {
        private:
            ScopeType type;
            /* Information only used for loops */
            Intermediate::label_index_t* loop_condition = nullptr;
            Intermediate::label_index_t* loop_end = nullptr;

            std::unordered_map<std::string, Intermediate::Variable*> variables = std::unordered_map<std::string, Intermediate::Variable*>();

        public:
            Scope(ScopeType type);
            /* Used for loops. Given the label names for the start of condition evaluation, and for the label
                at the end of the loop. */
            Scope(ScopeType type, Intermediate::label_index_t* condition, Intermediate::label_index_t* end);

            bool has_variable(std::string* name) const;
            /* Returns false if there was no instance, otherwise returns true and updates the variable info. */
            bool get_variable(std::string* name, Intermediate::Variable *&info) const;
            Intermediate::Variable *add_variable(std::string *name, Intermediate::Variable *info);
        
            Intermediate::label_index_t * get_loop_condition_label() const;
            Intermediate::label_index_t * get_loop_end() const;

            inline ScopeType get_type() const { return this->type; };
    };

    class ScopeManager {
        using VariableType = Intermediate::VariableType;

        private:
            Scope native_scope = Scope(ScopeType::NORMAL);

            void init_native_scope();

            std::vector<Scope> scopes = std::vector<Scope>();
            /* Pool of variable information. The manager handles memory for all variables, NOT
                the individual scopes. */
            std::vector<Intermediate::Variable*> variables = std::vector<Intermediate::Variable*>();

        public:
            ScopeManager();

            bool variable_exists(std::string* name) const;
            /* Returns false if there was no instance. Otherwise, returns true and updates the variable info.
                Goes up through the scope tree to try to find the variable. */
            bool get_variable(std::string* name, Intermediate::Variable *&info) const;
            /* Add a variable to the current scope. */
            Intermediate::Variable *add_variable(std::string* name, VariableType type, int function_index);

            bool last_scope_has_variable(std::string* name);

            bool in_function() const;

            void new_scope(ScopeType type);
            void new_scope(ScopeType type, Intermediate::label_index_t* condition, Intermediate::label_index_t* end);
            void pop_scope();

            // Get the start of the condition of the loop we're in
            // Returns nullptr if no loop is available
            Intermediate::label_index_t* get_loop_condition_label() const;
            // Get the end of the loop we're in
            // Returns nullptr if no loop is available
            Intermediate::label_index_t* get_loop_end() const;

            /**
                @param {VariableType::GLOBAL_MUTABLE | VariableType::GLOBAL_CONSTANT} - first type
                @return {VariableType} - The type, updated if we're in a function
            */
            VariableType add_variable_headers(VariableType basic_type);

            ~ScopeManager();
    };
}

#endif