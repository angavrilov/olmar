/*
 * \file    $Id$
 * \brief   Standard I/O
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#ifndef SEMANTICS_COMPILER
#include "console_serial.h"
#include "console_vga.h"
#include "cpu.h"
#include "memory.h"
#include "stdarg.h"
#endif

#define trace(T,format,...)                                                     \
do {                                                                            \
    register mword __esp asm ("esp");                                           \
    if (EXPECT_FALSE ((trace_mask & T) == T))                                   \
        printf ("[%d] " format "\n",                                            \
               ((__esp - 1) & ~0xfff) == KSTCK_SADDR ?                          \
                Cpu::id : ~0u, ## __VA_ARGS__);                                 \
} while (0)

#define NOOUTPUT
#ifdef NOOUTPUT
#undef trace
#define trace(T,format,...)
#endif

#ifndef SEMANTICS_COMPILER
/*
 * Definition of trace events
 */
enum {
    TRACE_CPU       = 1ul << 0,
    TRACE_VMX       = 1ul << 1,
    TRACE_SMX       = 1ul << 2,
    TRACE_APIC      = 1ul << 3,
    TRACE_MEMORY    = 1ul << 4,
    TRACE_PTAB      = 1ul << 5,
    TRACE_TIMER     = 1ul << 6,
    TRACE_INTERRUPT = 1ul << 7,
    TRACE_ACPI      = 1ul << 8,
    TRACE_KEYB      = 1ul << 9,
    TRACE_EC        = 1ul << 10,
    TRACE_PD        = 1ul << 11,
    TRACE_SYSCALL   = 1ul << 12,
    TRACE_DMAR      = 1ul << 13,
    TRACE_PCI       = 1ul << 14,
    TRACE_CONTEXT   = 1ul << 15
};

/*
 * Enabled trace events
 */
unsigned const trace_mask =
                            TRACE_CPU       |
#ifndef NDEBUG
                            TRACE_VMX       |
                            TRACE_SMX       |
                            TRACE_APIC      |
//                            TRACE_MEMORY    |
//                            TRACE_PTAB      |
                            TRACE_TIMER     |
//                            TRACE_INTERRUPT |
                            TRACE_ACPI      |
                            TRACE_KEYB      |
                            TRACE_EC        |
                            TRACE_PD        |
                            TRACE_SYSCALL   |
                            TRACE_DMAR      |
//                            TRACE_PCI       |
//                            TRACE_CONTEXT   |
#endif
                            0;

unsigned init_spinner (unsigned cpu);

FORMAT (1,0)
void vprintf (char const *format, va_list args);

FORMAT (1,2)
void printf (char const *format, ...);

FORMAT (1,2) NORETURN
void panic (char const *format, ...);

extern Console_serial serial;
extern Console_vga    screen;
#else
void panic (char const *format, ...) {};
#endif
