#pragma once

#include <string>
#include <tuple>

template <typename T>
using declaration = std::tuple<std::string, T>;
