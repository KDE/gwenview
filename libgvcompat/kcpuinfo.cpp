/*
 * This file is part of the KDE libraries
 * Copyright (C) 2003 Fredrik Höglund <fredrik@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <csignal>
#include <csetjmp>

#include <config.h>
#include "kcpuinfo.h"


#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define HAVE_GNU_INLINE_ASM
#endif


// Copied from kdecore/kglobal.h
#if __GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 > 2)
#  define KDE_NO_EXPORT __attribute__ ((visibility("hidden")))
#else
#  define KDE_NO_EXPORT
#endif

typedef void (*kde_sighandler_t) (int);

#ifdef __i386__
static jmp_buf KDE_NO_EXPORT env;

// Sighandler for the SSE OS support check
static void KDE_NO_EXPORT sighandler( int )
{
    std::longjmp( env, 1 );
}
#endif

#ifdef __PPC__
static sigjmp_buf KDE_NO_EXPORT jmpbuf;
static sig_atomic_t KDE_NO_EXPORT canjump = 0;

static void KDE_NO_EXPORT sigill_handler( int sig )
{
    if ( !canjump ) {
        signal( sig, SIG_DFL );
        raise( sig );
    }
    canjump = 0;
    siglongjmp( jmpbuf, 1 );
}
#endif

static int KDE_NO_EXPORT getCpuFeatures()
{
    int features = 0;

#if defined( HAVE_GNU_INLINE_ASM )
#if defined( __i386__ )
    bool haveCPUID = false;
    bool have3DNOW = false;
    int result = 0;

    // First check if the CPU supports the CPUID instruction
    __asm__ __volatile__(
    // Try to toggle the CPUID bit in the EFLAGS register
    "pushf                      \n\t"   // Push the EFLAGS register onto the stack
    "popl   %%ecx               \n\t"   // Pop the value into ECX
    "movl   %%ecx, %%edx        \n\t"   // Copy ECX to EDX
    "xorl   $0x00200000, %%ecx  \n\t"   // Toggle bit 21 (CPUID) in ECX
    "pushl  %%ecx               \n\t"   // Push the modified value onto the stack
    "popf                       \n\t"   // Pop it back into EFLAGS

    // Check if the CPUID bit was successfully toggled
    "pushf                      \n\t"   // Push EFLAGS back onto the stack
    "popl   %%ecx               \n\t"   // Pop the value into ECX
    "xorl   %%eax, %%eax        \n\t"   // Zero out the EAX register
    "cmpl   %%ecx, %%edx        \n\t"   // Compare ECX with EDX
    "je    .Lno_cpuid_support%= \n\t"   // Jump if they're identical
    "movl      $1, %%eax        \n\t"   // Set EAX to true
    ".Lno_cpuid_support%=:      \n\t"
    : "=a"(haveCPUID) : : "%ecx", "%edx" );

    // If we don't have CPUID we won't have the other extensions either
    if ( ! haveCPUID )
        return 0L;

    // Execute CPUID with the feature request bit set
    __asm__ __volatile__(
    "pushl  %%ebx               \n\t"   // Save EBX
    "movl      $1, %%eax        \n\t"   // Set EAX to 1 (features request)
    "cpuid                      \n\t"   // Call CPUID
    "popl   %%ebx               \n\t"   // Restore EBX
    : "=d"(result) : : "%eax", "%ecx" );

    // Test bit 23 (MMX support)
    if ( result & 0x00800000 )
        features |= KCPUInfo::IntelMMX;

    __asm__ __volatile__(
      "pushl %%ebx             \n\t"
      "movl $0x80000000, %%eax \n\t"
      "cpuid                   \n\t"
      "cmpl $0x80000000, %%eax \n\t"
      "jbe .Lno_extended%=     \n\t"
      "movl $0x80000001, %%eax \n\t"
      "cpuid                   \n\t"
      "test $0x80000000, %%edx \n\t"
      "jz .Lno_extended%=      \n\t"
      "movl      $1, %%eax     \n\t"   // // Set EAX to true
      ".Lno_extended%=:        \n\t"
      "popl   %%ebx            \n\t"   // Restore EBX
      : "=a"(have3DNOW) : );

    if ( have3DNOW )
        features |= KCPUInfo::AMD3DNOW;

#ifdef HAVE_X86_SSE
    // Test bit 25 (SSE support)
    if ( result & 0x00200000 ) {
        features |= KCPUInfo::IntelSSE;

        // OS support test for SSE.
        // Install our own sighandler for SIGILL.
        kde_sighandler_t oldhandler = std::signal( SIGILL, sighandler );

        // Try executing an SSE insn to see if we get a SIGILL
        if ( setjmp( env ) )
            features ^= KCPUInfo::IntelSSE; // The OS support test failed
        else
            __asm__ __volatile__("xorps %xmm0, %xmm0");

        // Restore the default sighandler
        std::signal( SIGILL, oldhandler );

        // Test bit 26 (SSE2 support)
        if ( (result & 0x00400000) && (features & KCPUInfo::IntelSSE) )
            features |= KCPUInfo::IntelSSE2;

        // Note: The OS requirements for SSE2 are the same as for SSE
        //       so we don't have to do any additional tests for that.
    }
#endif // HAVE_X86_SSE
#elif defined __PPC__ && defined HAVE_PPC_ALTIVEC
    signal( SIGILL, sigill_handler );
    if ( sigsetjmp( jmpbuf, 1 ) ) {
        signal( SIGILL, SIG_DFL );
    } else {
        canjump = 1;
        __asm__ __volatile__( "mtspr 256, %0\n\t"
                              "vand %%v0, %%v0, %%v0"
                              : /* none */
                              : "r" (-1) );
        signal( SIGILL, SIG_DFL );
        features |= KCPUInfo::AltiVec;
    }
#endif // __i386__
#endif //HAVE_GNU_INLINE_ASM

    return features;
}

unsigned int KCPUInfo::s_features = getCpuFeatures();


