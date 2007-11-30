/*
 * \file    $Id$
 * \brief   Capability
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

class Capability
{
    private:
        mword val;

    public:
        enum Type
        {
            INVALID,
            PD,
            WQ,
            EC,
            PT
        };

        Capability (Type t, void *p = 0) : val (reinterpret_cast<mword>(p) | t) {}

        ALWAYS_INLINE
        inline Type type() const
        {
            return static_cast<Type>(val & 7);
        }

        ALWAYS_INLINE
        inline void *ptr() const
        {
            return reinterpret_cast<void *>(val & ~7);
        }
};
