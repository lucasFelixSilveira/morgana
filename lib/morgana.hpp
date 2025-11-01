#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <variant>

namespace morgana {
    using dynamic = std::monostate;
    using non_size = std::variant<dynamic, int>;

    enum radical { Integer = 0 };

    /*
     * This class represents a type in the IR.
     * Where you can build your own types, such as arrays, structs, etc.
     *
     * Example:
     *
     *     type int32 = type::integer(32);
     *     type int32_ptr = int32.ptr();
     *     type int32_array = int32.vec(10);
     *     type int32_array_ptr = int32_array.ptr();
     */
    struct type {
    private:
        enum radical radical;
        non_size length;
        int bits;
        bool pointer;
        bool vector;
    public:
        type(enum radical radical, int bits): radical(radical), bits(bits), pointer(false), vector(false) {}

        /*
         * Constructor for integer type.
         * - You only need to specify the number of bits.
         */
        static type integer(int bits) {
            type t(radical::Integer, bits);
            return t;
        }

        /*
         * Constructor for pointer type.
         * - Used for create a simple pointer
         */
        type& ptr() {
            pointer = true;
            return *this;
        }

        /*
         * Constructor for vector type.
         * - You can create a simple array type (vector) with a given size.
         *   But, the size is not required. If not specified, the vector will be dynamic.
         * ================================================================
         * - WARNING!:
         * Keep in mind: The "dynamic size" is not dynamic. It means that the size will
         * be determined at compile time.
         */
        type& vec(non_size size) {
            length = size;
            vector = true;
            return *this;
        }

        /*
         * Make a Shared pointer type without a large code
         */
        std::shared_ptr<type> shared() {
            return std::make_shared<type>(*this);
        }

        /*
         * Convert the type class to the string representation
         * of the type in Morgana IR language.
         */
        std::string string() {
            std::stringstream ss;
            if( vector ) ss << "[";
            if( vector && std::holds_alternative<dynamic>(length) ) ss << "*:";
            if( vector && std::holds_alternative<int>(length) ) ss << std::get<int>(length) << ":";

            if( radical == Integer) {
                ss << "i" << bits;
            }

            if( vector ) ss << "]";
            if( pointer ) ss << "*";
            return ss.str();
        }
    };
}
