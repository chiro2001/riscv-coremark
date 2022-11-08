/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-03     Bernard      The first version
 */

#ifndef CPUPORT_H__
#define CPUPORT_H__

#include "rtconfig.h"

/**
 * SMP Coremark
 *   
 *    - (word) 0x8700_0004, 0x8700_0008, ... <= These are start flags for each core, when it is written non-zero value, then park!
 *    - (word) 0x8700_0104, 0x8700_0108, ... <= These are used for pass coremark iterate function parameter address!
 *    - (word) 0x8700_0204, 0x8700_0208, ... <= These are returned values!
 * 
 */

#define SMP_START_FLAG 0x87000000
#define SMP_PARAM_ADDR 0x87000100
#define SMP_FINISH_FLAG 0x87000200

/* bytes of register width  */
#ifdef ARCH_CPU_64BIT
#error "Have not support 64-bit yet"
#define STORE                   sd
#define LOAD                    ld
#define REGBYTES                8
#else
#define STORE                   sw
#define LOAD                    lw
#define REGBYTES                4
#endif

#define SRC_XIE         mie
#define SRC_XIP         mip
#define SRC_XTVEC       mtvec
#define SRC_XSTATUS     mstatus
#define SRC_XSCRATCH    mscratch
#define SRC_XEPC        mepc
#define SRC_XCAUSE      mcause
#define SRC_XTVAL       mtval
#define XRET            mret
#endif

