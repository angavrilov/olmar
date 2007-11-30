/*
 * \file    $Id$
 * \brief   IA32 Page Table
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "assert.h"
#ifndef SEMANTICS_COMPILER
#include "buddy.h"
#endif
#include "compiler.h"
#include "paging.h"
#include "slab.h"
#ifndef SEMANTICS_COMPILER
#include "types.h"
#endif

class Ptab : public Paging
{
    private:
        static Slab_cache cache;
        static Paddr remap_addr;

    public:
        ALWAYS_INLINE
        static inline void *operator new (size_t, bool small = false)
        {
            if (small)
                return cache.alloc();

            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        ALWAYS_INLINE
        static inline Ptab *master()
        {
            extern Ptab _master_l;
            return &_master_l;
        }

        ALWAYS_INLINE
        static inline Ptab *current()
        {
            mword addr;
            asm volatile ("mov %%cr3, %0" : "=r" (addr));
            return static_cast<Ptab *>(Buddy::phys_to_ptr (addr));
        }

        ALWAYS_INLINE
        inline void make_current()
        {
            assert (this);
            asm volatile ("mov %0, %%cr3" : : "r" (Buddy::ptr_to_phys (this)) : "memory");
        }

        void map (Paddr phys, mword virt, size_t size, Attribute attr = ATTR_NONE);

        void unmap (mword virt, size_t size);

        bool lookup (mword virt, Paddr &phys);

        void sync_local();

        size_t sync_master (mword virt);

        void sync_master_range (mword s_addr, mword e_addr);

        void *remap (Paddr phys);
};
