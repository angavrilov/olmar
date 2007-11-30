/*
 * \file    $Id$
 * \brief   Interrupt Vectors
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#define NUM_EXC         32
#define NUM_IRQ         16
#define NUM_GSI         48
#define VEC_MIN         (NUM_EXC + NUM_GSI)
#define VEC_APIC_THERM  (VEC_MIN + 0)
#define VEC_APIC_PERFM  (VEC_MIN + 1)
#define VEC_APIC_ERROR  (VEC_MIN + 2)
#define VEC_APIC_TIMER  (VEC_MIN + 3)
#define VEC_MAX         (VEC_MIN + 4)
