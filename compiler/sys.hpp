#pragma once

// sys.hpp
#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <iostream>

#ifdef _WIN32
    #include <intrin.h>  // __cpuid, __cpuidex
#elif defined(__linux__) || defined(__APPLE__)
    // Nada extra necessário para __cpuid intrínsecos do GCC/Clang
#endif

// =============== ENUM E TABELAS DE SYSCALLS ===============

enum class CSyscall {
    READ, WRITE, OPEN, CLOSE, EXIT, EXECVE, MMAP, MUNMAP,
    FORK, VFORK, SOCKET, BIND, LISTEN, ACCEPT, CONNECT,
    UNKNOWN
};

CSyscall syscallFromString(const std::string& name) {
    if (name == "read") return CSyscall::READ;
    if (name == "write") return CSyscall::WRITE;
    if (name == "open") return CSyscall::OPEN;
    if (name == "close") return CSyscall::CLOSE;
    if (name == "exit") return CSyscall::EXIT;
    if (name == "execve") return CSyscall::EXECVE;
    if (name == "mmap") return CSyscall::MMAP;
    if (name == "munmap") return CSyscall::MUNMAP;
    if (name == "fork") return CSyscall::FORK;
    if (name == "vfork") return CSyscall::VFORK;
    if (name == "socket") return CSyscall::SOCKET;
    if (name == "bind") return CSyscall::BIND;
    if (name == "listen") return CSyscall::LISTEN;
    if (name == "accept") return CSyscall::ACCEPT;
    if (name == "connect") return CSyscall::CONNECT;
    return CSyscall::UNKNOWN;
}

using SyscallTable = std::map<std::string, int>;

// =============== TABELAS DE SYSCALLS ===============

SyscallTable linux_x86_64_syscalls = {
    {"read", 0}, {"write", 1}, {"open", 2}, {"close", 3}, {"exit", 60},
    {"execve", 59}, {"mmap", 9}, {"munmap", 11}, {"fork", 57}, {"vfork", 58},
    {"socket", 41}, {"connect", 42}, {"accept", 43}, {"bind", 49}, {"listen", 50}
};

SyscallTable linux_i386_syscalls = {
    {"read", 3}, {"write", 4}, {"open", 5}, {"close", 6}, {"exit", 1},
    {"execve", 11}, {"mmap", 192}, {"munmap", 91}, {"fork", 2}
};

SyscallTable linux_aarch64_syscalls = {
    {"read", 63}, {"write", 64}, {"open", 56}, {"close", 57}, {"exit", 93},
    {"execve", 221}, {"mmap", 222}, {"munmap", 215}, {"fork", 220}, {"vfork", 219}
};

SyscallTable linux_arm_syscalls = {
    {"read", 3}, {"write", 4}, {"open", 5}, {"close", 6}, {"exit", 1},
    {"execve", 11}, {"mmap2", 192}, {"munmap", 91}
};

SyscallTable windows_x64_syscalls = {
    {"NtReadFile", 0x00}, {"NtWriteFile", 0x01}, {"NtOpenFile", 0x02},
    {"NtClose", 0x03}, {"NtCreateFile", 0x55}, {"NtTerminateProcess", 0x2C},
    {"NtAllocateVirtualMemory", 0x18}, {"NtFreeVirtualMemory", 0x1E}
};

// =============== STRUCT DE INFORMAÇÕES ===============

struct SystemInfo {
    std::string os;
    std::string arch;
    std::string isa;
    SyscallTable syscall_table;
};

// =============== FUNÇÃO DE CPUID MULTIPLATAFORMA ===============

inline void cpuid(int cpuinfo[4], int function_id) {
#ifdef _WIN32
    __cpuid(cpuinfo, function_id);
#else
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(cpuinfo[0]), "=b"(cpuinfo[1]), "=c"(cpuinfo[2]), "=d"(cpuinfo[3])
        : "a"(function_id), "c"(0)
    );
#endif
}

inline void cpuidex(int cpuinfo[4], int function_id, int subfunction_id) {
#ifdef _WIN32
    __cpuidex(cpuinfo, function_id, subfunction_id);
#else
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(cpuinfo[0]), "=b"(cpuinfo[1]), "=c"(cpuinfo[2]), "=d"(cpuinfo[3])
        : "a"(function_id), "c"(subfunction_id)
    );
#endif
}

// =============== DETECÇÃO COMPLETA ===============

SystemInfo detectSystemInfo() {
    SystemInfo info;

    // --- OS ---
#ifdef _WIN32
    info.os = "Windows";
#elif defined(__linux__)
    info.os = "Linux";
#elif defined(__APPLE__)
    info.os = "macOS";
#else
    info.os = "Unknown OS";
#endif

    // --- Arquitetura ---
#if defined(_M_X64) || defined(__x86_64__)
    info.arch = "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
    info.arch = "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
    info.arch = "ARM64";
#elif defined(_M_ARM) || defined(__arm__)
    info.arch = "ARM";
#else
    info.arch = "Unknown";
#endif

    // --- ISA (apenas x86/x64) ---
    info.isa = "Base";

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    int cpuinfo[4] = {0};

    cpuid(cpuinfo, 0);
    if (cpuinfo[0] >= 1) {
        cpuid(cpuinfo, 1);

        // EDX
        if (cpuinfo[3] & (1 << 25)) info.isa += ", SSE";
        if (cpuinfo[3] & (1 << 26)) info.isa += ", SSE2";

        // ECX
        if (cpuinfo[2] & (1 << 0))  info.isa += ", SSE3";
        if (cpuinfo[2] & (1 << 9))  info.isa += ", SSSE3";
        if (cpuinfo[2] & (1 << 19)) info.isa += ", SSE4.1";
        if (cpuinfo[2] & (1 << 20)) info.isa += ", SSE4.2";
        if (cpuinfo[2] & (1 << 28)) info.isa += ", AVX";

        // AVX2 e AVX512 (leaf 7)
        if (cpuinfo[0] >= 7) {
            cpuidex(cpuinfo, 7, 0);
            if (cpuinfo[1] & (1 << 5))  info.isa += ", AVX2";
            if (cpuinfo[1] & (1 << 16)) info.isa += ", AVX512F";
        }
    }
#endif

    if (info.isa.size() > 5 && info.isa[0] == ',') {
        info.isa = info.isa.substr(1);
    }
    if (info.isa == "Base" && info.isa.find(',') != std::string::npos) {
        info.isa = "Base";
    }

    // --- Tabela de Syscalls ---
    if (info.os == "Linux") {
        if (info.arch == "x86_64") info.syscall_table = linux_x86_64_syscalls;
        else if (info.arch == "x86") info.syscall_table = linux_i386_syscalls;
        else if (info.arch == "ARM64") info.syscall_table = linux_aarch64_syscalls;
        else if (info.arch == "ARM") info.syscall_table = linux_arm_syscalls;
    }
    else if (info.os == "Windows" && info.arch == "x86_64") {
        info.syscall_table = windows_x64_syscalls;
    }
    else if (info.os == "macOS") {
        std::cerr << "[!] macOS: syscalls não disponíveis diretamente (Mach traps)\n";
    }
    else {
        std::cerr << "[!] SO/Arch não suportado para syscalls: " << info.os << " " << info.arch << "\n";
    }

    return info;
}
