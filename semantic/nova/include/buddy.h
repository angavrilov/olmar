/*
 * \file    $Id$
 * \brief   Buddy Allocator
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "memory.h"
#include "spinlock.h"
#include "types.h"

#ifdef SEMANTICS_COMPILER
class Block
{
    public:
        Block *         prev;
        Block *         next;
        unsigned short  ord;
        unsigned short  tag;

        enum {
            Used  = 0,
            Free  = 1
        };
};
#endif

class Buddy
{
#ifndef SEMANTICS_COMPILER
    private:
#else
    public:
#endif
#ifndef SEMANTICS_COMPILER
        class Block
        {
            public:
                Block *         prev;
                Block *         next;
                unsigned short  ord;
                unsigned short  tag;

                enum {
                    Used  = 0,
                    Free  = 1
                };
        };
#endif

        Spinlock        lock;
        signed long     max_idx;
        signed long     min_idx;
        mword           p_base;
        mword           l_base;
        unsigned        order;
        Block *         index;
        Block *         head;

        ALWAYS_INLINE
        inline signed long block_to_index (Block *b)
        {
            return b - index;
        }

        ALWAYS_INLINE
        inline Block *index_to_block (signed long i)
        {
            return index + i;
        }

        ALWAYS_INLINE
        inline signed long page_to_index (mword l_addr)
        {
            return l_addr / PAGE_SIZE - l_base / PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword index_to_page (signed long i)
        {
            return l_base + i * PAGE_SIZE;
        }

        ALWAYS_INLINE
        inline mword virt_to_phys (mword virt)
        {
            return virt - l_base + p_base;
        }

        ALWAYS_INLINE
        inline mword phys_to_virt (mword phys)
        {
            return phys - p_base + l_base;
        }

    public:
        enum Fill
        {
            NOFILL,
            FILL_0,
            FILL_1
        };

        static Buddy allocator;

        INIT
        Buddy (mword phys, mword virt, mword f_addr, size_t size);

        void *alloc (unsigned short ord, Fill fill = NOFILL);

        void free (mword addr);

        ALWAYS_INLINE
        static inline void *phys_to_ptr (Paddr phys)
        {
            return reinterpret_cast<void *>(allocator.phys_to_virt (static_cast<mword>(phys)));
        }

        ALWAYS_INLINE
        static inline mword ptr_to_phys (void *virt)
        {
            return allocator.virt_to_phys (reinterpret_cast<mword>(virt));
        }
};
