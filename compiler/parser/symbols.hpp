#pragma once

#include "contexts.hpp"
#include "type.hpp"
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
#include "types/integer.hpp"
#include "types/bool.hpp"
#include "types/strong_alias.hpp"

#define MCU_READ_WRITE_CONTEXTS { context::MCU_GPIO_INSTRUCTION, context::MCU_TURN_INSTRUCTION, context::MCU_READ_INSTRUCTION }

struct morgana_subtypes { int instruction; std::string identifier; type real_one;};
using morgana_types = std::variant<morgana_integer, morgana_bool, morgana_strong_alias>;

struct function_data { std::vector<std::string> types; };

using named_integers = std::tuple<std::string, int>;

enum symbolKind { GPIO_PIN };
struct morgana_allocation;
using symbol = std::variant<
    std::monostate,  // No data entries
    morgana_integer, // Integer type
    morgana_bool,    // Boolean type

    /* "Strong alias" is just a different form of the same type,
     * but with a different name and different uses.
     * § The stored value is the same as the original type, but
     * you can't access it directly. */
    morgana_strong_alias,

    /* Below that comment, all data types are morgana internal.
     *
     * That means: The user cannot access these data. These data
     * are here just for the Morgana compiler know all the symbols
     * in the user code during the compilation phase. */

    function_data,      // Store the type of the arguments of the function
    named_integers,     // Storage integer types
    morgana_allocation  // Storage the data of the instruction of allocation
>;

struct morgana_allocation { std::string identifier; std::shared_ptr<symbol> type; };

class SymbolTable {
private:
    std::stack<std::unordered_map<std::string, symbol>> scopes;

public:
    SymbolTable() {
        enter_scope();
        current_scope().insert({
            {"u8",    morgana_integer(8,   false) },
            {"u16",   morgana_integer(16,  false) },
            {"u32",   morgana_integer(32,  false) },
            {"u64",   morgana_integer(64,  false) },
            {"u128",  morgana_integer(128, false) },
            {"u256",  morgana_integer(256, false) },

            {"i8",    morgana_integer(8,   true) },
            {"i16",   morgana_integer(16,  true) },
            {"i32",   morgana_integer(32,  true) },
            {"i64",   morgana_integer(64,  true) },
            {"i128",  morgana_integer(128, true) },
            {"i256",  morgana_integer(256, true) },

            {"gpio_pin", morgana_strong_alias(morgana_integer(16, false), MCU_READ_WRITE_CONTEXTS) }
        });
    }

    void enter_scope() {
        scopes.push(std::unordered_map<std::string, symbol>());
    }

    void exit_scope() {
        if( scopes.size() > 1 ) scopes.pop();
    }

    std::unordered_map<std::string, symbol>& current_scope() {
        return scopes.top();
    }

    bool insert(std::string& name, symbol data) {
        auto& scope = current_scope();
        if( scope.find(name) != scope.end() ) return false;
        scope[name] = data;
        return true;
    }

    std::optional<symbol> lookup(std::string name) {
        auto temp_stack = scopes;

        while(!temp_stack.empty()) {
            auto& scope = temp_stack.top();
            auto it = scope.find(name);
            if( it != scope.end() ) return it->second;
            temp_stack.pop();
        }
        return std::nullopt;
    }

    std::optional<symbol> lookup_current(const std::string& name) {
        auto& scope = current_scope();
        auto it = scope.find(name);
        if( it != scope.end() ) return it->second;
        return std::nullopt;
    }

    bool exists(std::string& name) {
        return lookup(name).has_value();
    }

    bool exists_in_current(std::string& name) {
        return lookup_current(name).has_value();
    }

    bool remove(std::string& name) {
        auto& scope = current_scope();
        return scope.erase(name) > 0;
    }

    size_t scope_level() const {
        return scopes.size() - 1;
    }
};

SymbolTable symbol_table;
