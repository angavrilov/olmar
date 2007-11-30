/*
 * \file    $Id$
 * \brief   Slab Allocator
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "bits.h"
#include "buddy.h"
#include "compiler.h"
#include "spinlock.h"
#include "types.h"

class Slab;

class Slab_cache
{
    private:
        Spinlock    lock;
        Slab *      curr;
        Slab *      head;

        /*
         * Back end allocator
         */
        void grow();

    public:
        unsigned size;  // Size of an element
        unsigned buff;  // Size of an element buffer (includes link field)
        unsigned elem;  // Number of elements

        Slab_cache (char const *name, size_t elem_size, unsigned elem_align);

        /*
         * Front end allocator
         */
        void *alloc();

        /*
         * Front end deallocator
         */
        void free (void *ptr);

        /*
         * Back end deallocator
         */
        void reap();
};

class Slab
{
    public:
        unsigned        avail;
        Slab_cache *    cache;
        Slab *          prev;                     // Prev slab in cache
        Slab *          next;                     // Next slab in cache
        char *          head;

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            Buddy::allocator.free (reinterpret_cast<mword>(ptr));
        }

        Slab (Slab_cache *slab_cache);

        ALWAYS_INLINE
        inline bool full() const
        {
            return !avail;
        }

        ALWAYS_INLINE
        inline bool empty() const
        {
            return avail == cache->elem;
        }

        void enqueue();
        void dequeue();

        ALWAYS_INLINE
        inline void *alloc();

        ALWAYS_INLINE
        inline void free (void *ptr);
};
