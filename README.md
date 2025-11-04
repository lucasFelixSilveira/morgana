# Moragana IR language
##
### What is Moragana?
Moragana is a IR language projected to change LLVM on Carla project. But, you can use on your on compiler.

## How better Morgana is than LLVM
- LLVM Code
```llvm
define i32 @main(i8 %0, ptr %1) {
entry:
  %3 = alloca i8, align 1
  %4 = alloca ptr, align 8
  store i8 %0, ptr %3, align 1
  store ptr %1, ptr %4, align 8
  ret i32 0
}
```
- The same code in Morgana
```morgana
i32 main i8 [*:i8]* {
  (a0, a1) @_
  ret 0
}
```

## And about the lib?
- How can i use the lib to generate Morgana IR?
```cpp
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

    morgana::type i8 = morgana::type::integer(8);
    morgana::type i32 = morgana::type::integer(32);
    morgana::type pvecstr = morgana::type::integer(8).ptr().vec(morgana::dynamic());

    morgana::function f("main", i32.shared(), morgana::function::args{
        i8.shared(),
        pvecstr.shared()
    }, context.string());

    builder << f.string();
    std::cout << builder.string(); // Print the generated IR
    return 0;
}
```
