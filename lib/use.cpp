#include "morgana/builder.hpp"
#include "morgana/context.hpp"
#include "morgana/storage.hpp"
#include "morgana.hpp"
#include <iostream>
#include <vector>

int main() {
    Storage storage;
    Builder builder(false);
    int addr;

    Context context;
    morgana::desconstruct d(morgana::mics::that, {});
    context << d.string();

    auto int32 = morgana::type::integer(32);

    morgana::alloc ptr(storage, int32.vec(12).shared());
    context << ptr.string();

    morgana::setindex index(ptr, morgana::value);
    context << index.string();

    context << morgana::ret::string();
    morgana::function f("main", morgana::type::integer(32).shared(), {}, context.string());

    builder << f.string();
    storage.leave();
    std::cout << storage.string() << builder.string();
    return 0;
}
