#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <algorithm>

#if defined(_WIN32)
    #include <intrin.h>
#elif defined(__linux__) || defined(__ANDROID__)
    #include <sys/auxv.h>
#endif

#if defined(__arm__) || defined(__aarch64__)
    #if defined(__linux__)
        #include <asm/hwcap.h>
    #endif
#endif

enum class CSyscall {
    READ, WRITE, OPEN, CLOSE, EXIT, EXECVE, MMAP, MUNMAP,
    FORK, VFORK, SOCKET, BIND, LISTEN, ACCEPT, CONNECT,
    UNKNOWN
};

CSyscall syscallFromString(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "read")    return CSyscall::READ;
    if (lower == "write")   return CSyscall::WRITE;
    if (lower == "open")    return CSyscall::OPEN;
    if (lower == "close")   return CSyscall::CLOSE;
    if (lower == "exit")    return CSyscall::EXIT;
    if (lower == "execve")  return CSyscall::EXECVE;
    if (lower == "mmap")    return CSyscall::MMAP;
    if (lower == "munmap")  return CSyscall::MUNMAP;
    if (lower == "fork")    return CSyscall::FORK;
    if (lower == "vfork")   return CSyscall::VFORK;
    if (lower == "socket")  return CSyscall::SOCKET;
    if (lower == "bind")    return CSyscall::BIND;
    if (lower == "listen")  return CSyscall::LISTEN;
    if (lower == "accept")  return CSyscall::ACCEPT;
    if (lower == "connect") return CSyscall::CONNECT;
    return CSyscall::UNKNOWN;
}

using SyscallTable = std::map<std::string, int>;

static const SyscallTable linux_x86_64 = {
    {"read", 0}, {"write", 1}, {"open", 2}, {"close", 3}, {"exit", 60},
    {"execve", 59}, {"mmap", 9}, {"munmap", 11}, {"fork", 57}, {"vfork", 58},
    {"socket", 41}, {"connect", 42}, {"accept", 43}, {"bind", 49}, {"listen", 50}
};

static const SyscallTable linux_i386 = {
    {"read", 3}, {"write", 4}, {"open", 5}, {"close", 6}, {"exit", 1},
    {"execve", 11}, {"mmap", 192}, {"munmap", 91}, {"fork", 2}
};

static const SyscallTable linux_aarch64 = {
    {"read", 63}, {"write", 64}, {"openat", 56}, {"close", 57}, {"exit", 93},
    {"execve", 221}, {"mmap", 222}, {"munmap", 215}, {"fork", 220}, {"vfork", 219},
    {"socket", 198}, {"connect", 203}, {"accept", 202}, {"bind", 200}, {"listen", 201}
};

static const SyscallTable linux_arm = {
    {"read", 3}, {"write", 4}, {"open", 5}, {"close", 6}, {"exit", 1},
    {"execve", 11}, {"mmap2", 192}, {"munmap", 91}, {"fork", 2}
};

static const SyscallTable windows_nt_x64 = {
    {"NtReadFile", 6}, {"NtWriteFile", 8}, {"NtOpenFile", 0x55},
    {"NtClose", 0x0C}, {"NtCreateFile", 0x55}, {"NtTerminateProcess", 0x2C},
    {"NtAllocateVirtualMemory", 0x18}, {"NtFreeVirtualMemory", 0x1E}
};

struct SystemInfo {
    std::string os = "Unknown";
    std::string arch = "Unknown";
    SyscallTable syscall_table;

    bool has_sse     = false;
    bool has_sse2    = false;
    bool has_sse3    = false;
    bool has_ssse3   = false;
    bool has_sse41   = false;
    bool has_sse42   = false;
    bool has_avx     = false;
    bool has_avx2    = false;
    bool has_avx512f = false;

    bool has_neon = false;
    bool has_sve  = false;
};

inline void cpuid(int cpuinfo[4], int leaf, int subleaf = 0) {
#if defined(_WIN32)
    if (subleaf == 0) __cpuid(cpuinfo, leaf);
    else              __cpuidex(cpuinfo, leaf, subleaf);
#else
    __asm__ volatile (
        "cpuid"
        : "=a"(cpuinfo[0]), "=b"(cpuinfo[1]),
          "=c"(cpuinfo[2]), "=d"(cpuinfo[3])
        : "a"(leaf), "c"(subleaf)
    );
#endif
}

inline SystemInfo detectSystemInfo() {
    SystemInfo info;

#if defined(_WIN32)
    info.os = "Windows";
#elif defined(__linux__) || defined(__ANDROID__)
    info.os = "Linux";
#elif defined(__APPLE__)
    info.os = "macOS";
#endif

#if defined(__x86_64__) || defined(_M_X64)
    info.arch = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    info.arch = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.arch = "aarch64";
#elif defined(__arm__)
    info.arch = "arm";
#endif

    if (info.os == "Linux") {
        if      (info.arch == "x86_64")  info.syscall_table = linux_x86_64;
        else if (info.arch == "x86")     info.syscall_table = linux_i386;
        else if (info.arch == "aarch64") info.syscall_table = linux_aarch64;
        else if (info.arch == "arm")     info.syscall_table = linux_arm;
    }
    else if (info.os == "Windows" && info.arch == "x86_64") {
        info.syscall_table = windows_nt_x64;
    }

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    int regs[4]{};
    cpuid(regs, 0);
    int max_leaf = regs[0];

    if (max_leaf >= 1) {
        cpuid(regs, 1);
        info.has_sse     = (regs[3] & (1 << 25)) != 0;
        info.has_sse2    = (regs[3] & (1 << 26)) != 0;
        info.has_sse3    = (regs[2] & (1 << 0))  != 0;
        info.has_ssse3   = (regs[2] & (1 << 9))  != 0;
        info.has_sse41   = (regs[2] & (1 << 19)) != 0;
        info.has_sse42   = (regs[2] & (1 << 20)) != 0;
        info.has_avx     = (regs[2] & (1 << 28)) != 0;

        if (max_leaf >= 7) {
            cpuid(regs, 7, 0);
            info.has_avx2    = (regs[1] & (1 << 5))  != 0;
            info.has_avx512f = (regs[1] & (1 << 16)) != 0;
        }
    }
#endif

#if defined(__linux__) || defined(__ANDROID__)
    #if defined(__arm__) || defined(__aarch64__)
        unsigned long hwcap = getauxval(AT_HWCAP);

        #if defined(__aarch64__)
            info.has_neon = (hwcap & (1UL << 1)) != 0;
        #else
            info.has_neon = (hwcap & (1UL << 12)) != 0;
        #endif

        #ifdef HWCAP_SVE
            info.has_sve = (hwcap & HWCAP_SVE) != 0;
        #endif
    #endif
#endif

    return info;
}
