#pragma once

#include "integer.hpp"
#include "bool.hpp"
#include <variant>
#include <vector>

using morgana_strong_alias = std::tuple<
    std::variant<std::monostate, morgana_integer, morgana_bool>,
    std::vector<int>
>;
