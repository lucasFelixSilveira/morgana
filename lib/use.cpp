#include "morgana/builder.hpp"
#include "morgana.hpp"

int main() {
    Builder builder(true);
    morgana::type t = morgana::type::integer(32).ptr().vec(4);
    builder.symbols.add(builder, "_", t.shared());
    return 0;
}
