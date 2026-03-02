#pragma once

#include <string>

#if defined(__linux__) || defined(__ANDROID__)
    #include <sys/auxv.h>
    #include <asm/hwcap.h>
#endif

// ======================================================
// DETECÇÃO DE ARQUITETURA
// ======================================================

#if defined(__i386__) || defined(__x86_64__)
    #define ARCH_X86
#elif defined(__aarch64__) || defined(__arm__)
    #define ARCH_ARM
#else
    #define ARCH_UNKNOWN
#endif


// ======================================================
// CPUID (apenas x86 / x86_64)
// ======================================================

#ifdef ARCH_X86

    #if defined(_MSC_VER)
        #include <intrin.h>
    #else
        #include <cpuid.h>
    #endif

    inline void cpuid(int cpuinfo[4], int function_id) {
    #if defined(_MSC_VER)
        __cpuid(cpuinfo, function_id);
    #else
        __cpuid(function_id,
                cpuinfo[0],
                cpuinfo[1],
                cpuinfo[2],
                cpuinfo[3]);
    #endif
    }

#endif


// ======================================================
// STRUCT DE SISTEMA
// ======================================================

struct SystemInfo {

    std::string os;
    std::string arch;

    // x86 features
    bool sse     = false;
    bool sse2    = false;
    bool avx     = false;
    bool avx2    = false;

    // ARM features
    bool neon    = false;
    bool sve     = false;
};


// ======================================================
// DETECÇÃO COMPLETA
// ======================================================

inline SystemInfo detectSystemInfo() {

    SystemInfo info;

    // =============================
    // SISTEMA OPERACIONAL
    // =============================

    #if defined(_WIN32)
        info.os = "Windows";
    #elif defined(__ANDROID__)
        info.os = "Android";
    #elif defined(__linux__)
        info.os = "Linux";
    #elif defined(__APPLE__)
        info.os = "Apple";
    #else
        info.os = "Unknown";
    #endif


    // =============================
    // x86 / x86_64
    // =============================

    #ifdef ARCH_X86

        #if defined(__x86_64__)
            info.arch = "x86_64";
        #else
            info.arch = "x86";
        #endif

        int cpuinfo[4] = {0};

        // Feature flags (leaf 1)
        cpuid(cpuinfo, 1);

        info.sse  = (cpuinfo[3] & (1 << 25)) != 0;
        info.sse2 = (cpuinfo[3] & (1 << 26)) != 0;
        info.avx  = (cpuinfo[2] & (1 << 28)) != 0;

        // AVX2 (leaf 7)
        cpuid(cpuinfo, 7);
        info.avx2 = (cpuinfo[1] & (1 << 5)) != 0;

    #endif


    // =============================
    // ARM / ARM64
    // =============================

    #ifdef ARCH_ARM

        #if defined(__aarch64__)
            info.arch = "ARM64";
        #else
            info.arch = "ARM";
        #endif

        #if defined(__linux__) || defined(__ANDROID__)

            unsigned long hwcaps = getauxval(AT_HWCAP);

            #ifdef HWCAP_NEON
                info.neon = (hwcaps & HWCAP_NEON) != 0;
            #endif

            #ifdef HWCAP_SVE
                info.sve = (hwcaps & HWCAP_SVE) != 0;
            #endif

        #endif

    #endif


    // =============================
    // DESCONHECIDO
    // =============================

    #ifdef ARCH_UNKNOWN
        info.arch = "Unknown";
    #endif


    return info;
}