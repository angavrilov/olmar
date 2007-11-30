/*
 * \file    $Id$
 * \brief
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#define STRING(x) #x
#define EXPAND(x) STRING(x)

#ifdef SEMANTICS_COMPILER
#define __builtin_expect(X,Y)  (X)
void __builtin_trap() {};
#define __attribute__(X)
#endif

#if defined(__GNUC__)

        #define COMPILER            "gcc "__VERSION__

        #define EXPECT_FALSE(X)     __builtin_expect((X), false)
        #define EXPECT_TRUE(X)      __builtin_expect((X), true)

#ifndef SEMANTICS_COMPILER
// declare __builtin_trap() for poor elsa
void __builtin_trap(void);
#endif

        #define UNREACHED           __builtin_trap()

        #define ALIGNED(X)          __attribute__((aligned(X)))
        #define ALWAYS_INLINE       __attribute__((always_inline))
        #define CONST               __attribute__((const))
        #define CPULOCAL
        #define FORMAT(X,Y)         __attribute__((format (printf, (X),(Y))))
        #define INIT                __attribute__((section (".init")))
        #define INITDATA            __attribute__((section (".initdata")))
        #define INIT_PRIORITY(X)    __attribute__((init_priority((X))))
        #define NOINLINE            __attribute__((noinline))
        #define NONNULL             __attribute__((nonnull))
        #define NORETURN            __attribute__((noreturn))
        #define PURE                __attribute__((pure))
        #define REGPARM(X)          __attribute__((regparm(X)))
        #define STDCALL             __attribute__((stdcall))
        #define UNUSED              __attribute__((unused))
        #define USED                __attribute__((used))
        #define WARN_UNUSED_RESULT  __attribute__((warn_unused_result))
        #define WEAK                __attribute__((weak))

#endif
