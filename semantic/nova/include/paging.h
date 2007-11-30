/*
 * \file    $Id$
 * \brief   IA32 Paging Support
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "types.h"

class Paging
{
    public:
        enum Error
        {
            ERROR_PROT              = 1u << 0,      // 0x1
            ERROR_WRITE             = 1u << 1,      // 0x2
            ERROR_USER              = 1u << 2,      // 0x4
            ERROR_RESERVED          = 1u << 3,      // 0x8
            ERROR_IFETCH            = 1u << 4       // 0x10
        };

        enum Attribute
        {
            ATTR_NONE               = 0,
            ATTR_PRESENT            = 1ull << 0,    // 0x1
            ATTR_WRITABLE           = 1ull << 1,    // 0x2
            ATTR_USER               = 1ull << 2,    // 0x4
            ATTR_WRITE_THROUGH      = 1ull << 3,    // 0x8
            ATTR_UNCACHEABLE        = 1ull << 4,    // 0x10
            ATTR_ACCESSED           = 1ull << 5,    // 0x20
            ATTR_DIRTY              = 1ull << 6,    // 0x40
            ATTR_SUPERPAGE          = 1ull << 7,    // 0x80
            ATTR_GLOBAL             = 1ull << 8,    // 0x100
            ATTR_NOEXEC             = 1ull << 63,   // 0x8000000000000000

            ATTR_PTAB               = ATTR_ACCESSED |
                                      ATTR_USER     |
                                      ATTR_WRITABLE |
                                      ATTR_PRESENT,

            ATTR_LEAF               = ATTR_DIRTY    |
                                      ATTR_ACCESSED |
                                      ATTR_PRESENT,

            ATTR_INVERTED           = ATTR_NOEXEC,

            ATTR_ALL                = ATTR_NOEXEC | ((1ull << 12) - 1)
        };

        static unsigned const levels            = 3;
        static unsigned const bits_per_level    = 9;
        static Attribute ptab_bits[levels] asm ("ptab_bits");

        INIT
        static void check_features();

        INIT
        static void enable();

    protected:
        Paddr val;

        ALWAYS_INLINE
        inline Paddr addr() const
        {
            return val & ~ATTR_ALL;
        }

        ALWAYS_INLINE
        inline Attribute attr() const
        {
            return Attribute (val & ATTR_ALL);
        }

        ALWAYS_INLINE
        inline bool present() const
        {
            return val & ATTR_PRESENT;
        }

        ALWAYS_INLINE
        inline bool superpage() const
        {
            return val & ATTR_SUPERPAGE;
        }
};
