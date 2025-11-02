#include "morgana/builder.hpp"
#include "morgana/context.hpp"
#include "morgana.hpp"
#include <iostream>
#include <vector>

int main() {
    Builder builder(false);

    morgana::desconstruct::values data = {"argc", "argv"};

    Context context;
    morgana::desconstruct d(morgana::mics::that, data);
    context << d.string();

    morgana::function f("main", morgana::type::integer(32).shared(), morgana::function::args{
        morgana::type::integer(8).shared(),
        morgana::type::integer(8).ptr().vec(morgana::dynamic()).shared()
    }, context.string());

    builder << f.string();
    std::cout << builder.string();
    return 0;
}
