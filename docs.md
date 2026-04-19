# Morgana Documentation

- [Compiler](#compiler)
    - [CLI](#cli)
    - [Extensors](#extensors)
- [Libraries](#morgana-codegen-libraries)
    - [Builder](#builder)
- [IR Language Syntax](#morgana-ir-language) 
    - [Declarations](#morgana-declarations)
    - [Instructions](#morgana-instructions)
        - [comptime](#comptime)
        - [alloc](#alloc)
        - [store](#store)
        - [constant](#constant)
        - [puts](#puts)

<hr>

# Compiler

The Morgana compiler has some very interesting features. It works quite differently from other IR compilers, such as the `llvm-compiler` or `llc`.

**Main differences:**
- Optimizations depend on the chosen [Extensors](#extensors) system.
- Simpler errors with less information, making it easier to understand the problem.
- `Other distinctions should be added.`

## CLI

The CLI follows this basic syntax:

```sh-session
$ morgana <command> <optional arguments> <flags>
```

Currently, the available commands are:

- `build` - Compiles Morgana code
- `install` - Allows the installation of a new [Extensor](#extensors)
    - **Arguments**: `morgana install <extensor name> <branch>`

```sh-session
$ morgana install aarch64 linux
```

- `Other commands should be added.`

We also have several flags that can be used (even if useless in some cases) with any command:

- `-m` - Tells Morgana that the file to be compiled is `main.morg`. It will throw an error if it cannot find the source file. Use `-m` to specify a different main project file.
- `-o` - Morgana always tries to use your machine’s architecture as the target by default. However, you can use cross-compiling. The `-o` flag not only specifies which [Extensor](#extensors) to use, but also changes the entire build process, using the appropriate compilers for both your host machine and the target machine.
- `-v` - The compilation process often depends not only on Morgana, but also on external compilers and tools (for example, `avr-gcc` and `avr-dude` for Arduino). Morgana hides all logs from these external programs by default. Use `-v` to display these logs.
- `-n` - Some build systems, such as `idf.py` (ESP32), require a project name. To avoid generic names, the `-n` flag allows you to specify a custom name. By default, Morgana will try to read the name from the `target.toml` file.
- `Other flags should be added.`

## Extensors

Extensors are one of the most interesting aspects of Morgana. Morgana is an almost complete compiler, but it does not include a built-in code generation system. Instead, you download the codegen externally. This makes the Morgana clone faster, the project smaller, and much more modular — which means it is also more scalable.

Extensors are written in Lua. [Learn more about how Extensors work or how to create one here](https://github.com/Carla-Corp/extensors).

Morgana uses [Runa](https://github.com/lucasFelixSilveira/runa) as the embedded Lua interpreter for running Extensors.

You can download an Extensor using the `install` command:

```sh-session
$ morgana install aarch64 ios
```

# Morgana codegen libraries

Morgana uses a simple system for generating intermediate code in its compiler. It provides lightweight classes that allow you to call the .string() method to generate source code within a context builder.

Currently, we have official libraries for:
- [C++](https://github.com/Carla-Corp/morgana-codegen-cpp)

## Builder

A Builder is a simple structure that lets you append strings sequentially while also storing some additional metadata.
You can call `.string()` on a Builder to retrieve all its internal context as text. This "internal context" represents the resulting IR code produced by your compiler.

You can save this output to a file and then compile it using the Morgana compiler, or simply use the built-in library method `.compile()` on the builder. Note that after calling the compile method, the builder will be destroyed (deconstructed).    

## Context

A Context is essentially a Builder, but with two key differences:

It does not have the `.compile()` method.
It can override or push content into its parent builder’s context.

For example, a function that requires a real stack frame must be built inside a Context (not a regular Builder). This allows the Context to be “pushed” into the parent Builder’s context when needed.

# Morgana IR language

Morgana’s syntax is similar to LLVM’s. Its main difference lies in keeping types implicit and being more descriptive: instead of a single reusable operation, it offers a dedicated operation for each case. This makes the language harder to learn, but easier to read. 

## Morgana Declarations 

This is the basic syntax for variable assignment in Morgana. You assign the result of an expression directly to a variable without needing to declare its type explicitly.

```morgana
x = expression
```

### Morgana Instructions

In Morgana, instructions are classified as either **declarative** or **non-declarative**.

- Instructions marked as **non-declarative** must be used as standalone statements (outside of any expression or declaration).
- Instructions marked as **declarative** must be used inside a declaration.
- Instructions marked as **constant** must be used inside a constant declaration.
- If no classification is specified, the instruction can be used in any context.

<hr>
    
### Comptime
**THIS INSTRUCTION IS SPECIAL!**

That instruction is 100% controlled by your extensor. Remember check your extensor documentation before do anything with this instruction.

**ALL OF THE OFFICIAL EXTENSORS HAVE:**
- `_start` - Make the entry for the main function

##

- `non-declarative`
- `identifier` - The identifier of the comptime expression 

```morgana
comptime identifier
``` 

> Does it generate any reference block?  
**No**

<hr>

#### Alloc
- `declarative`
- `identifier` – Part of the declaration
- `type` – The type to be allocated on the stack

```morgana
identifier = alloc type
```

> Does it generate any reference block?  
**Yes.** It belongs to the allocations block.

<hr>

#### Store 
- `non-declarative` 
- `identifier` – An identifier that **already exists** in the allocation blocks.
- `value` – A value compatible with the allocated type. You can also use `default`, constant identifiers, or loads.

```morgana
store identifier value
```

> Does it generate any reference block?  
**No**

<hr>

#### Constant
- `declarative`
- `identifier` – Part of the declaration
- `value` - Can be a literal String or a literal number.

```morgana
identifier = constant value
```

> Does it generate any reference block?  
**Yes.** It belongs to the constants block.

<hr>
    
#### Puts
- `non-declarative`
- `identifier` – An identifier of an already defined constant
```morgana
puts identifier
```

> Does it generate any reference block?  
**No**
