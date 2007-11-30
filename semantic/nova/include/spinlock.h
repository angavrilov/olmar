/*
 * \file    $Id$
 * \brief   Generic Spinlock
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "types.h"

class Spinlock
{
    private:
        uint8 _lock;

    public:
        ALWAYS_INLINE
#ifndef SEMANTICS_COMPILER
        inline Spinlock() : _lock (1) {}
#else
        inline Spinlock() { _lock = 1; }
#endif

        ALWAYS_INLINE
        inline void lock()
        {
            asm volatile ("1:   lock; decb %0;  "
                          "     jns 3f;         "
                          "2:   pause;          "
                          "     cmpb $0, %0;    "
                          "     jle 2b;         "
                          "     jmp 1b;         "
                          "3:                   "
                          : "+m" (_lock) : : "memory");
        }

        ALWAYS_INLINE
        inline void unlock()
        {
            asm volatile ("movb $1, %0" : "=m" (_lock) : : "memory");
        }
};
