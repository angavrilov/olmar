/*
 * \file    $Id$
 * \brief   Bit Scan Functions
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "assert.h"
#include "compiler.h"
#include "types.h"

ALWAYS_INLINE
inline int bit_scan_reverse (unsigned val)
{
    if (EXPECT_FALSE (!val))
        return -1;

    asm volatile ("bsr %1, %0" : "=r" (val) : "rm" (val));

    return val;
}

ALWAYS_INLINE
inline uint64 div64 (uint64 n, uint32 d, uint32 *r)
{
    uint64 q;
    asm volatile ("divl %5;"
                  "xchg %1, %2;"
                  "divl %5;"
                  "xchg %1, %3;"
                  : "=A" (q),   // quotient
                    "=r" (*r)   // remainder
                  : "a"  (static_cast<uint32>(n >> 32)),
                    "d"  (0),
                    "1"  (static_cast<uint32>(n)),
                    "rm" (d));
    return q;
}

ALWAYS_INLINE
static inline unsigned align_down (mword val, unsigned align)
{
    val &= ~(align - 1);                // Expect power-of-2
    return val;
}

ALWAYS_INLINE
static inline unsigned align_up (mword val, unsigned align)
{
    val += (align - 1);                 // Expect power-of-2
    return align_down (val, align);
}
