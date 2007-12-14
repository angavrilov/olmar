/* Marcus 
 *
 * This file replaces nova/include/stdio.h
 *
 * all debug functions need to be declared as empty macros
 */
#pragma once

#define trace(T, format, ...)
#define panic(format, ...)
#define printf(format, ...)
#define vprintf(format, args)

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

unsigned const trace_mask = 0;
