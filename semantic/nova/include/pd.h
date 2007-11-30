/*
 * \file    $Id$
 * \brief   Protection Domain
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "slab.h"
#include "space_io.h"
#include "space_mem.h"
#include "space_obj.h"
#include "types.h"

class Pd : public Space_mem, public Space_io, public Space_obj
{
    private:
        // PD Cache
        static Slab_cache cache;

    public:
        // Current PD
        static Pd *current CPULOCAL;

        // Kernel PD
        static Pd kern;

        // Allocator
        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return cache.alloc();
        }

        // Deallocator
        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            cache.free (ptr);
        }

        // Constructor
        Pd (char const *name);

        // Constructor (Pd::kern)
        Pd (Ptab *mst) : Space_mem (mst), Space_io (this), Space_obj (this) {}

        void make_current();

        ALWAYS_INLINE
        static inline void init_cpu_ptab (Ptab *ptab)
        {
            kern.percpu[Cpu::id] = ptab;
        }
};
