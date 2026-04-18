#pragma once

#include "symbols.hpp"
#include <stack>
#include <string>
#include <vector>

#define MORGANA_BLOCK_INTERNALS \
    X(allocations, allocation) \

struct block {
    template<typename T>
    using block_t = std::stack<std::vector<T>>;

    /* Global stacks for parser */

    /* Stack that stores allocations in the current parsing context.
     *
     * Each block level contains a vector of (identifier, symbol),
     * representing declared allocations within that scope. */
    using allocation_t = std::tuple<std::string, symbol>;
    using allocations_t = block_t<allocation_t>;
    inline static allocations_t allocations;

    /* Pushes a new empty vector onto the stack.
     * This vector becomes the current active block.
     *
     * After calling this, any insertion will affect
     * this new top-level block until it is popped. */
    template<typename T>
    static void push(block_t<T>& b) { b.push(std::vector<T>()); }

    /* Pushes a new block into all registered global stacks.
     *
     * This function is intended for synchronized scope handling,
     * where multiple block stacks must advance together.
     *
     * Add new global stacks here as needed. */
    static void push_generic() {
        #define X(type, ...) push(type);
        MORGANA_BLOCK_INTERNALS;
        #undef X
    }

    /* Removes the current block from the stack.
     *
     * After popping, the previous block (if any)
     * becomes the new active block.
     *
     * Calling this on an empty stack is undefined behavior. */
    template<typename T>
    static void pop(block_t<T>& b) { b.pop(); }

    /* Pops the current block from all registered global stacks.
     *
     * This must mirror push_generic(), ensuring all stacks
     * remain structurally consistent. */
    static void pop_generic() {
        #define X(type, ...) pop(type);
        MORGANA_BLOCK_INTERNALS;
        #undef X
    }

    /* Inserts a value into the current block.
     *
     * The value is appended to the vector at the
     * top of the stack.
     *
     * Calling this on an empty stack is undefined behavior. */
    template<typename T>
    static void push_back(block_t<T>& b, const T& value) { b.top().push_back(value); }

    /* Returns true if there are no blocks in the stack.
     *
     * When empty, no operations that depend on b.top()
     * should be performed. */
    template<typename T>
    static bool empty(block_t<T>& b) { return b.empty(); }

    /* Emits a fatal error related to block usage.
     *
     * This function attempts to infer the block type name
     * at compile-time using the MORGANA_BLOCK_INTERNALS macro.
     *
     * The 'cause' string should describe the invalid operation
     * or identifier that triggered the error. */
    template<typename T>
    static void error(T b, std::string cause) {
        std::string what;

        #define X(name, singular) if constexpr (std::is_same_v<T, name##_t>) what = #singular;
        MORGANA_BLOCK_INTERNALS;
        #undef X

        CompilerOutputs::Fatal("Morgana blocks error! `" + cause + "` don't exist or isn't a valid " + what + " block identifier");
    }

    /* Initializes all global stacks.
     *
     * Ensures that each stack starts with at least one
     * valid block, avoiding undefined behavior when
     * accessing b.top() early in execution. */
    static void init() {
        #define X(name, ...) if( name.size() == 0 ) name.push({});
        MORGANA_BLOCK_INTERNALS;
        #undef X
    }

    /* Returns true if the current block contains the given
     * identifier. You need to provide a type T that supports
     * equality comparison with std::string.
     *
     * If T is not comparable with std::string, this function
     * will always return false.
     *
     * To support other types, provide a specialization of
     * this function that implements the appropriate lookup
     * logic for T.
     *
     * Only checks the top-most (current) block, not parent scopes. */
    template<typename T>
    static bool lookup(block_t<T>& b, std::string identifier) {
        if( b.empty() ) return false;

        if constexpr (std::is_same_v<T, allocation_t>)
        /* -> */ for( const auto& item : b.top() )
        /* -> */    if( std::get<0>(item) == identifier ) return true;

        return false;
    }

    template<typename T>
    static T peek(block_t<T>& b, std::string identifier) {
        if constexpr (std::is_same_v<T, allocation_t>)
        /* -> */ for( const auto& item : b.top() )
        /* -> */    if( std::get<0>(item) == identifier ) return item;

        return T();
    }
};
