#include <string>
#include <variant>

#define NON_COMPLEX_NODE(keyword, kind) \
case keyword: { \
    results.push_back({ kind, std::monostate() }); \
} continue;

#define COMPLEX_NODE(keyword, kind, ret, block) \
case keyword: { \
    auto result = [&]() -> ret { \
        block \
    }(); \
    results.push_back({ kind, result }); \
} continue;

using ret = std::variant<std::monostate, int, std::string>;
