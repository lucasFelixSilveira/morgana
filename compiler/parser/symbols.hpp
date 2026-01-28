#pragma once

#include "type.hpp"
#include <vector>

struct morgana_types { type value; std::string regex; };

struct function_data { std::vector<std::string> types; };
