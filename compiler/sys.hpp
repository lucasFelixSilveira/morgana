#pragma once

#include <string>

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

struct SystemInfo {
    std::string os = "Unknown";
    std::string arch = "Unknown";

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
