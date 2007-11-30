/*
 * \file    $Id$
 * \brief   Memory Space
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "config.h"
#include "cpu.h"
#include "ptab.h"
#include "types.h"

class Exc_state;

class Space_mem
{
    protected:
        Ptab *master;
        Ptab *percpu[NUM_CPU];

    public:
        // Constructor
        Space_mem();

        // Constructor for Pd::kern memory space
        Space_mem (Ptab *mst) : master (mst) {}

        ALWAYS_INLINE
        inline Ptab *mst_ptab() const
        {
            return master;
        }

        ALWAYS_INLINE
        inline Ptab *cpu_ptab() const
        {
            return percpu[Cpu::id];
        }

        ALWAYS_INLINE
        inline void mem_map_local (Paddr phys, mword virt, size_t size, Paging::Attribute attr)
        {
            percpu[Cpu::id]->map (phys, virt, size, attr);
        }

        ALWAYS_INLINE
        inline void insert (Paddr phys, mword virt, size_t size, Paging::Attribute attr)
        {
            master->map (phys, virt, size, attr);
        }

        ALWAYS_INLINE
        inline void remove (mword virt, size_t size)
        {
            master->unmap (virt, size);
        }

        ALWAYS_INLINE
        inline bool lookup (mword virt, Paddr &phys)
        {
            return master->lookup (virt, phys);
        }

        static void page_fault (Exc_state *xs);
};
