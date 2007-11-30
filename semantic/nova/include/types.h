/*
 * \file    $Id$
 * \brief   Constant-Width Types
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#ifndef SEMANTICS_COMPILER
#include <stddef.h>
#else
typedef unsigned long size_t;
#endif

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef unsigned long long  uint64;

typedef unsigned long       mword;
typedef unsigned long long  Paddr;
