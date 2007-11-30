/*
 * \file    $Id$
 * \brief   Object Space
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "capability.h"
#include "compiler.h"
#include "types.h"

class Exc_state;
class Space_mem;

class Space_obj
{
    private:
        Space_mem *space_mem;

        ALWAYS_INLINE
        inline mword idx_to_virt (unsigned idx)
        {
            return OBJSP_SADDR + idx * sizeof (Capability);
        }

    public:
        ALWAYS_INLINE
        inline Space_obj (Space_mem *space) : space_mem (space) {}

        ALWAYS_INLINE
        inline Capability lookup (unsigned idx)
        {
            return *reinterpret_cast<Capability *>(idx_to_virt (idx));
        }

        bool insert (unsigned idx, Capability cap);
        void remove (unsigned idx);

        static void page_fault (Exc_state *xs);
};
