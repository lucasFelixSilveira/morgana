#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

extern "C" {
#define EVA_USING_C_ABI
#include "eva.h"
}

class eva {
    EvaParser *parser;
public:
    eva(std::string path)
        : parser(eva_make_parser(path.c_str())) {}

    template <typename T>
    using result = std::pair<bool, T>;

    template <typename T>
    static T check(std::pair<bool, T> p) {
        return std::get<0>(p);
    }

    template <typename T>
    static T data(std::pair<bool, T> p) {
        return std::get<1>(p);
    }

    class map {
        EvaValue values;
    public:
        map(EvaValue values)
            : values(values) {}

        map() {};

        const size_t size() const {
            return eva_get_map_length(values);
        }

        template<typename T>
        const std::pair<bool, T> operator[](std::string key) const {
            if(! eva_check_exist_field_in_map(values, key.c_str()) )
            /* -> */ return std::make_pair(false, T());

            EvaValue value = eva_get_map_field(values, key.c_str());

            if constexpr (std::is_same_v<T, std::string>) {
                if( value.tag == eva_string )
                    return {true, std::string(value.data.string)};

                throw std::runtime_error("unexpected field type: " + key);
            }

            if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int> || std::is_same_v<T, size_t>) {
                if( value.tag == eva_number )
                    return {true, static_cast<T>(value.data.number)};

                throw std::runtime_error("unexpected field type: " + key);
            }

            if constexpr (std::is_same_v<T, bool>) {
                if( value.tag == eva_bool )
                    return {true, static_cast<T>(value.data.boolean)};

                throw std::runtime_error("unexpected field type: " + key);
            }

            if constexpr (std::is_same_v<T, list>) {
                if( value.tag == eva_list )
                    return {true, value};

                throw std::runtime_error("unexpected field type: " + key);
            }

            if constexpr (std::is_same_v<T, map>) {
                if( value.tag == eva_map )
                    return {true, value};

                throw std::runtime_error("unexpected field type: " + key);
            }

        }
    };

    class list {
        EvaValue values;
    public:
        list(EvaValue values)
            : values(values) {}

        list() {};

        const size_t size() const {
            return eva_get_list_length(values);
        }

        template<typename T>
        const std::pair<bool, T> operator[](size_t index) const {
            if( index >= size() ) return std::make_pair(false, T());
            EvaValue value = eva_get_list_field(values, index);

            if constexpr (std::is_same_v<T, std::string>) {
                if( value.tag == eva_string )
                    return {true, std::string(value.data.string)};

                throw std::runtime_error("unexpected index type: " + std::to_string(index));
            }

            if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int> || std::is_same_v<T, size_t>) {
                if( value.tag == eva_number )
                    return {true, static_cast<T>(value.data.number)};

                throw std::runtime_error("unexpected index type: " + std::to_string(index));
            }

            if constexpr (std::is_same_v<T, bool>) {
                if( value.tag == eva_bool )
                    return {true, static_cast<T>(value.data.boolean)};

                throw std::runtime_error("unexpected index type: " + std::to_string(index));
            }

            if constexpr (std::is_same_v<T, list>) {
                if( value.tag == eva_list )
                    return {true, value};

                throw std::runtime_error("unexpected index type: " + std::to_string(index));
            }

            if constexpr (std::is_same_v<T, map>) {
                if( value.tag == eva_map )
                    return {true, value};

                throw std::runtime_error("unexpected index type: " + std::to_string(index));
            }

        }
    };


    ~eva() {
        free(parser->parser);
    }

    template <typename T>
    std::pair<bool, T> get(std::string ns, std::string field) {
        if(! eva_check_exist_field_in_namespace(parser, ns.c_str(), field.c_str()) )
        /* -> */ return std::make_pair(false, T());

        if constexpr (std::is_same_v<T, std::string>) {
            EvaValue value = eva_get_value_from_namespace(parser, ns.c_str(), field.c_str());

            if( value.tag == eva_string )
                return {true, std::string(value.data.string)};

            throw std::runtime_error("unexpected field type: " + field);
        }

        if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int> || std::is_same_v<T, size_t>) {
            EvaValue value = eva_get_value_from_namespace(parser, ns.c_str(), field.c_str());

            if( value.tag == eva_number )
                return {true, static_cast<T>(value.data.number)};

            throw std::runtime_error("unexpected field type: " + field);
        }

        if constexpr (std::is_same_v<T, bool>) {
            EvaValue value = eva_get_value_from_namespace(parser, ns.c_str(), field.c_str());

            if( value.tag == eva_bool )
                return {true, static_cast<T>(value.data.boolean)};

            throw std::runtime_error("unexpected field type: " + field);
        }

        if constexpr (std::is_same_v<T, list>) {
            EvaValue value = eva_get_value_from_namespace(parser, ns.c_str(), field.c_str());

            if( value.tag == eva_list )
                return {true, value};

            throw std::runtime_error("unexpected field type: " + field);
        }

        if constexpr (std::is_same_v<T, map>) {
            EvaValue value = eva_get_value_from_namespace(parser, ns.c_str(), field.c_str());

            if( value.tag == eva_map )
                return {true, value};

            throw std::runtime_error("unexpected field type: " + field);
        }

        throw std::runtime_error("unsupported type to simple get");
    }
};
