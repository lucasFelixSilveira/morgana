# Moragana IR language <img align="right" src="./assets/icon_nobg.png" alt="Morgana Logo" width="65" height="65">

Morgana is a lightweight, high-performance Intermediate Representation designed to cut through the heavy, bloated pipeline of LLVM.
It powers the Carla compiler, but is fully usable as a standalone IR for any custom language or code generation tool.

### 🚀 Why Morgana Exists

LLVM is slow.

Yes, it's insanely powerful and feature-heavy — but that complexity comes with a price: massive pipelines, heavy abstractions, and sluggish code generation.

Morgana strips away the unnecessary layers and gives you a direct, nearly-metal IR, with a tiny translation pipeline that turns instructions into machine code with minimal overhead.

If you want real speed without drowning in LLVM’s complexity,

**Morgana isn’t an upgrade — it’s the alternative.**

##

## ✨ What Is Morgana?

Morgana is a clean and compact IR language originally built to replace LLVM in the Carla compiler project.

But nothing stops you from using it in your own compiler or VM.
It was designed to be simple enough to generate, powerful enough to optimize, and low-level enough to map cleanly to native assembly.

## 🔥 LLVM vs Morgana
  
- **LLVM IR**
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

- The same code in **Morgana IR**
```morgana
i32 main i8 [*:i8]* {
  (a0, a1) @_
  ret 0
}
```

Notice the difference:
**Same semantics, 5× less noise.**

# 📚 Library Usage Example

```cpp
#include "morgana/builder.hpp"
#include "morgana/context.hpp"
#include "morgana.hpp"
#include <iostream>
#include <vector>

int main() {
    Builder builder(false);

    morgana::desconstruct::values data = {};

    Context context;
    morgana::desconstruct d(morgana::mics::that, data);
    context << d.string();

    morgana::type i8 = morgana::type::integer(8);
    morgana::type i32 = morgana::type::integer(32);
    morgana::type pvecstr = morgana::type::integer(8).ptr().vec(morgana::dynamic());

    morgana::function f("main", i32.shared(), morgana::function::args{}, context.string());

    builder << f.string();
    std::cout << builder.string(); // Print the generated IR
    return 0;
}
```
