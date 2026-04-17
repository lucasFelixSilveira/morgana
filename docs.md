# Morgana Documentation

- [Compiler](#compiler)
    - [CLI](#cli)
    - [Extensors](#extensors)
- [Libraries](#libraries) - Not written yet
- [IR Language Syntax](#morgana-language) - Not written yet

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
