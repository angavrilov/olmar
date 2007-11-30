/*
 * \file    $Id$
 * \brief   Translation Lookaside Buffer (TLB)
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "types.h"

class Tlb
{
    public:
        ALWAYS_INLINE
        static inline void flush()
        {
            mword cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        ALWAYS_INLINE
        static inline void flush (mword virt)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<mword *>(virt)));
        }
};
