/*
 * \file    $Id$
 * \brief   Virtual-Memory Layout
 * \author  Udo Steinberg <udo@hypervisor.org>
 *
 * Copyright (C) 2005-2007, Udo Steinberg <udo@hypervisor.org>
 * Technische Universitaet Dresden, Operating Systems Group
 * All rights reserved.
 */

#pragma once

#define PAGE_BITS       12
#define PAGE_SIZE       0x1000
#define PAGE_MASK       (PAGE_SIZE - 1)

#define LOAD_ADDR       0x200000
#define BOOT_ADDR       0x204000
#define LINK_ADDR       0xc0000000

/*
 * Global Range from 0xc0000000 to 0xcfc00000
 */

/* HW Devices */
#define HWDEV_EADDR     0xcfdfe000

/* LT Space */
#define LTSPC_SADDR     0xcfdfe000
#define LTSPC_EADDR     LTSPC_SADDR + PAGE_SIZE

/* VGA Console */
#define VGACN_SADDR     0xcfdff000
#define VGACN_EADDR     VGACN_SADDR + PAGE_SIZE

/*
 * CPU Local Range from 0xcfe00000 to 0xd0000000
 */
#define LOCAL_SADDR     0xcfe00000
#define LOCAL_EADDR     0xd0000000

/* Local APIC */
#define LAPIC_SADDR     0xcfffc000
#define LAPIC_EADDR     LAPIC_SADDR + PAGE_SIZE

/* Kernel Stack */
#define KSTCK_SADDR     0xcfffe000
#define KSTCK_EADDR     KSTCK_SADDR + PAGE_SIZE

/* CPU-Local */
#define CPULC_SADDR     0xcffff000
#define CPULC_EADDR     CPULC_SADDR + PAGE_SIZE

/*
 * AS Local Range from 0xd0000000 to max
 */

/* I/O Space */
#define IOBMP_SADDR     0xd0000000
#define IOBMP_EADDR     IOBMP_SADDR + PAGE_SIZE * 2

/* Object Space */
#define OBJSP_SADDR     0xe0000000
#define OBJSP_EADDR     0xf0000000

/* Remap Window */
#define REMAP_SADDR     0xff000000
#define REMAP_EADDR     0xff800000
