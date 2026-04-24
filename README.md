<img align="right" src="./assets/icon_nobg.png" alt="Morgana Logo" width="120px" height="120px">
<br><br>

# 🐑 Morgana IR language 

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

## Morgana Extensors

What is extensors?

Extensors are extensions to Morgana codegen, which allow you to add your custom instructions, types, and optimizations to the language.

- `I need implement my own extensors?`: No, you can use the official extensors, third-party extensors or even write your own.
- `How can i write my own extensors?`: You can write your own extensors using [Runa (ルナ)](https://github.com/lucasFelixSilveira/runa) and the pattern who is teach on the [Morgana Extensors](https://github.com/Carla-corp/extensors) repository.
- `I can write without overriding any of the existing extensors?`: Yes, you can write your own extensors without overriding any of the existing ones. Compiling with the flag `-o`

```sh-session
# x86_64 extensor for example
$ morgana build -o x86_64-optimized
```

## What official extensors are already implemented?
<table>
    <tr>
        <td></td>
        <td>Linux</td>
        <td>Windows</td>
        <td>Android</td>
        <td>MacOS / IOS</td>
        <td>Embeded boards</td>
    </tr>
    <tr>
        <td>x86_64</td>
        <td>🟡 (In progress)</td>
        <td>🟡 (Just cross-compiling)</td>
        <td> - </td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>i3286</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>aarch64</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>arm</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>risc-V</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>xtensa</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>🟠 (Pendent)</td>
    </tr>
    <tr>
        <td>atmega</td>
        <td> - </td>
        <td> - </td>
        <td> - </td>
        <td> - </td>
        <td>🟠 (Pendent)</td>
    </tr>
    <tr>
        <td>NVIDIA</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>AMD (GPU)</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>Intel (GPU)</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    </tr>
    <tr>
        <td>AMD (GPU)</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    <tr>
    <tr>
        <td>Adreno</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
        <td>❌</td>
    <tr>
</table>

- [See documentation here](./docs.md)
