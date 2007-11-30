/*
 * \file    $Id$
 * \brief   I/O Space
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "types.h"

class Exc_state;
class Space_mem;

class Space_io
{
    private:
        Space_mem *space_mem;

        ALWAYS_INLINE
        inline mword idx_to_virt (unsigned idx)
        {
            return IOBMP_SADDR + idx / 8;
        }

        ALWAYS_INLINE
        inline mword idx_to_bit (unsigned idx)
        {
            return idx % (8 * sizeof (mword));
        }

    public:
        ALWAYS_INLINE
        inline Space_io (Space_mem *space) : space_mem (space) {}

        void insert (unsigned idx);
        void remove (unsigned idx);

        static void page_fault (Exc_state *xs);
};
