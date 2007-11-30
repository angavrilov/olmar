/*
 * \file    $Id$
 * \brief
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#include "compiler.h"
#include "stdio.h"

#ifdef NDEBUG
#define assert(X)
#else
#define assert(X)   do {                                                                            \
                        if (EXPECT_FALSE (!(X)))                                                    \
                            panic ("Assertion \"%s\" failed at %s:%d\n", #X, __FILE__, __LINE__);   \
                    } while (0)
#endif
