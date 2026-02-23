#pragma once

#include <sstream>
#include <string>

struct type {
public:
    enum radical { Common, Array };
    radical kind;
    std::string value;
    int size = 0;
    bool ptr = false;

    static type common(bool ptr, std::string value) {
        return type(ptr, Common, value);
    }

    static type array(bool ptr, std::string value, int size) {
        return type(ptr, Array, value, size);
    }

    type() = default;
    type(bool ptr, radical kind, std::string value, int size = 0) : ptr(ptr), kind(kind), value(value), size(size) {}

    int bytes() const {
        switch (kind) {
            case Common:
                if( value == "i8" )   return 1;
                if( value == "i16" )  return 2;
                if( value == "i32" )  return 4;
                if( value == "i64" )  return 8;
                if( value == "u8" )   return 1;
                if( value == "u16" )  return 2;
                if( value == "u32" )  return 4;
                if( value == "u64" )  return 8;
                if( value == "f32" )  return 4;
                if( value == "f64" )  return 8;
                if( value == "void" ) return 0;
            case Array:
                return size * type::common(false, value).bytes();
            default: return -1;
        }
    }

    int matrixPos() const {
        if( value == "i8" )   return 3;
        if( value == "i16" )  return 2;
        if( value == "i32" )  return 1;
        if( value == "i64" )  return 0;
        if( value == "u8" )   return 3;
        if( value == "u16" )  return 2;
        if( value == "u32" )  return 1;
        if( value == "u64" )  return 0;
        return -1;
    }

    std::string json() {
        std::stringstream ss;
        ss << "{ ";
        ss << " \"bytes\": " << bytes() << ", ";
        ss << " \"matrix\": " << (matrixPos() + 1) << ", ";
        ss << " \"ptr\": " << (ptr ? "true" : "false") << ", ";

        switch(kind) {
            case Array: {
                ss << " \"kind\": \"Array\", ";
                ss << " \"size\": " << size << ", ";
                ss << " \"type\": \"" << value << "\" ";
            } break;

            case Common: {
                ss << " \"kind\": \"Common\", ";
                ss << " \"type\": \"" << value << "\" ";
            } break;
        }

        ss << "}";
        return ss.str();
    }
};
