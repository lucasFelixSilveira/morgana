#include "morgana/builder.hpp"
#include "morgana/context.hpp"
#include "morgana/storage.hpp"
#include "morgana.hpp"
#include <iostream>
#include <vector>

int main() {
    Storage storage;
    Builder builder(false);

    morgana::desconstruct::values data = {"argc", "argv"};

    Context context;
    morgana::desconstruct d(morgana::mics::that, data);
    context << d.string();

    auto asciz = morgana::type::integer(8).ptr();
    auto vecAsciz = morgana::type::clone(storage, asciz).vec(morgana::dynamic());

    morgana::function f("main", morgana::type::integer(32).shared(), morgana::function::args{
        morgana::type::integer(8).shared(),
        morgana::type::clone(storage, vecAsciz).ptr().shared()
    }, context.string());

    builder << f.string();
    std::cout << storage.string() << builder.string();
    return 0;
}
