#pragma once

#include "type.hpp"
#include <variant>
#include <vector>

struct morgana_types { type value; std::string regex; };
struct morgana_subtypes { int instruction; std::string identifier; type real_one;};

using morgana_paramters = std::variant<morgana_types, morgana_subtypes>;

struct function_data { std::vector<std::string> types; };
