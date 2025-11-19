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

    morgana::alloc ptr(storage, int32.shared());
    context << ptr.string();

    morgana::store store(ptr.shared(), 32);
    context << store.string();

    morgana::load load(storage, ptr.shared());
    context << load.save(&addr).string();

    morgana::function f("main", morgana::type::integer(32).shared(), {}, context.string());

    builder << f.string();
    storage.leave();
    std::cout << storage.string() << builder.string();
    return 0;
}
