#pragma once
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include "compiler_outputs.hpp"
#include "params.hpp"

#define SSWITCH(val) auto _v = (val); do {
#define SIS(val) } while(0); if( _v == (val) ) { do {
#define SDEFAULT() } while(0); do {
#define SEND } while(0);

#ifdef _WIN32
#include <windows.h>
#include <string>
std::string NIL_FD_BACKEND = " > NUL 2>&1 ";

#define CAND(x, y) (__cmd_exists_win(#x) && (y))
inline bool __cmd_exists_win(const std::string& name) {
    DWORD size = GetEnvironmentVariableA("PATH", nullptr, 0);
    if( size == 0 ) return false;

    std::string path(size, '\0');
    GetEnvironmentVariableA("PATH", path.data(), size);
    size_t start = 0;
    while (true) {
        size_t end = path.find(';', start);
        std::string dir = path.substr(start, end - start);

        if(! dir.empty() && dir.front() == '"' ) dir.erase(0, 1);
        if(! dir.empty() && dir.back() == '"' ) dir.pop_back();

        std::string full = dir + "\\" + name + ".exe";
        DWORD attr = GetFileAttributesA(full.c_str());
        if( attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY) ) return true;

        if( end == std::string::npos ) break;
        start = end + 1;
    }

    return false;
}


#else
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <string>
std::string NIL_FD_BACKEND = " > /dev/null 2>&1 ";
#define CAND(x, y) (__cmd_exists_unix(#x) && (y))
inline bool __cmd_exists_unix(const std::string& name) {
    const char* path = std::getenv("PATH");
    if(! path ) return false;

    std::stringstream ss(path);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        std::string full = dir + "/" + name;
        if( access(full.c_str(), X_OK) == 0 ) return true;
    }

    return false;
}
#endif

const bool end = true;

struct Backend {
    static void assemble(CompilerParams& params, std::string s, std::string o, std::string exe, std::string *suffix) {
        std::stringstream breaker;
        breaker << "\n" << Colorizer::DARK_GREY << "└─ " << Colorizer::RESET;

        auto const LINUX = 0, WINDOWS = 1, MACOS = 2;
        auto sys = detectSystemInfo();
        auto checkout = sys.os == "linux" ? LINUX : (sys.os == "macos" ? MACOS : WINDOWS);

        #define DEFINE_SUFFIXES(x, y, z)                           \
        std::function<std::string()> func = [&]() -> std::string { \
            if( params.output == "native-bin" )     return x;      \
            if( params.output == "shared-object" )  return y;      \
            if( params.output == "static-archive" ) return z;      \
            return x;                                              \
        };                                                         \
        *suffix += func();                                         \
        exe += *suffix;

        switch (checkout) {
            case LINUX: {
                DEFINE_SUFFIXES(std::string(), ".so", ".a");

                /* Compile from x86_64-linux without cross-compilation.
                 *
                 * 1. Use 'as' to compile the Morgana IR assembly output to an object file.
                 * 2. Use 'gcc' to compile the C FFI source to an object file (if enabled).
                 * 3. Use 'ld' or 'ar' to link the object files together into an executable,
                 *    shared-object or a static-archive.
                 */
                if( sys.arch == "x86_64" && params.target == "x86_64-linux" ) {
                    std::string as = "as \"" + s + "\" -o \"" + o + "\"" + NIL_FD_BACKEND;
                    if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile Morgana IR to object file using as");

                    if( params.output == "native-bin" || params.output == "shared-object" ) {
                        std::string ld = "ld " + std::string(params.output == "native-bin" ? "" : "-shared ") + "\"" + o + "\" -o \"" + exe + "\"" + NIL_FD_BACKEND;
                        if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link Morgana IR to elf file using ld");
                        return;
                    }

                    if( params.output == "static-archive" ) {
                        std::string ld = "ar rcs \"" + exe + "\" \"" + o + "\"" + NIL_FD_BACKEND;
                        if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link Morgana IR to static library file using ar");
                        return;
                    }
                };

                /* Cross-compilation for x86_64-windows using MinGW-w64 toolchain.
                 *
                 * 1. Use 'as' to assemble Morgana IR output to COFF object file.
                 * 2. Use 'objcopy' to fix symbols.
                 * 3. Use 'gcc' as linker driver to produce final executable.
                 * 4. If C FFI enabled, compile C source and link together.
                 */
                if( sys.arch == "x86_64" && params.target == "x86_64-windows" ) {
                    bool mingw =
                        CAND(x86_64-w64-mingw32-gcc,
                        CAND(x86_64-w64-mingw32-as,
                        CAND(x86_64-w64-mingw32-objcopy, end)));

                    if(! mingw ) CompilerOutputs::Fatal("Failed to find Mingw-w64 toolchain. Install it and try again." + breaker.str() + "https://www.mingw-w64.org/downloads/");

                    std::string as = "x86_64-w64-mingw32-as --64 \"" + s + "\" -o \"" + o + ".old\"" + (params.verbose ? "" : NIL_FD_BACKEND);
                    if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to assemble to COFF");

                    std::string objcopy = "x86_64-w64-mingw32-objcopy \"" + o + ".old\" \"" + o + "\"" + (params.verbose ? "" : NIL_FD_BACKEND);
                    if( std::system(objcopy.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to convert COFF to ELF");

                    std::string libpath = "/usr/x86_64-w64-mingw32/lib";
                    FILE* find = popen("find /usr -name \"libmsvcrt.a\" 2>/dev/null | grep \"x86_64-w64-mingw32\" | head -1", "r");
                    if( find ) {
                        char buf[256];
                        if( fgets(buf, sizeof(buf), find) ) {
                            std::string path(buf);
                            path.erase(path.find_last_not_of(" \n\r\t") + 1);
                            size_t pos = path.find_last_of('/');
                            if( pos != std::string::npos ) libpath = path.substr(0, pos);
                        }
                        pclose(find);
                    }

                    std::string link_flags = "-L\"" + libpath + "\" -lmsvcrt -lkernel32 -lmingw32 -lmingwex -Wl,--image-base,0x140000000";

                    std::string ld = "x86_64-w64-mingw32-gcc -m64 \"" + o + "\" -o \"" + exe + "\" " + link_flags + " -Wl,--subsystem,console -Wl,--entry,main";
                    if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link executable");
                    return;
                }
            } break;

            case WINDOWS: {
                /* Compile from x86_64-windows without cross-compilation.
                 *
                 * 1. Use 'as' to compile the Morgana IR assembly output to an object file.
                 * 2. Use 'objcopy' to fix symbols.
                 * 3. Use 'gcc' to compile the C FFI source to an object file (if enabled).
                 * 4. Use 'gcc' to link the object files together into an executable.
                 */
                if( sys.arch == "x86_64" && params.target == "x86_64-windows" ) {
                    bool mingw =
                        CAND(x86_64-w64-mingw32-gcc,
                        CAND(x86_64-w64-mingw32-as,
                        CAND(x86_64-w64-mingw32-objcopy, end)));
                    if(! mingw ) CompilerOutputs::Fatal("Failed to find Mingw-w64 toolchain. Install it and try again." + breaker.str() + "use: winget install -e --id MartinStorsjo.LLVM-MinGW.UCRT");

                    std::string as = "x86_64-w64-mingw32-as -c \"" + s + "\" -o \"" + o + ".old\"" + (params.verbose ? "" : NIL_FD_BACKEND);
                    if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to assemble to COFF");

                    std::string objcopy = "x86_64-w64-mingw32-objcopy \"" + o + ".old\" \"" + o + "\"" + (params.verbose ? "" : NIL_FD_BACKEND);
                    if( std::system(objcopy.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to convert COFF");

                    std::string ld = "x86_64-w64-mingw32-gcc -nostartfiles -m64 \"" + o + "\" -o \"" + exe + "\" -mconsole -Wl,--subsystem,console -Wl,--entry,main";
                    if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link executable");

                    return;
                }
            } break;
        }
    }
};
