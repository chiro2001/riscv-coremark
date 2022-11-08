/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018/10/01     Bernard      The first version
 * 2018/12/27     Jesven       Add SMP support
 * 2020/6/12      Xim          Port to QEMU and remove SMP support
 */

#define XSTATUS_FS         (3 << 13) /* initial state of FPU, clear to disable */
#define XSTATUS_PUM        (1 << 18)
#include "cpuport.h"
#include "encoding.h"

#if __riscv_xlen == 64
# define LREG ld
# define SREG sd
# define REGBYTES 8
#else
# define LREG lw
# define SREG sw
# define REGBYTES 4
#endif

boot_hartid: .int
  .global      boot_hartid

  .global   _start
  .section ".start", "ax"
_start:
  // for sifive uart
  li a0,0x54000000
	// disable interrupts
	li a1,0
	sw a1,16(a0)
	// enable tx
	li a1,1
	sw a1,8(a0)
	// enable rx
	li a1,1
	sw a1,12(a0)

	// print START\n
	addi a1,zero,'C'
	addi a2,zero,'M'
	addi a3,zero,'0'
	addi a4,zero,'\n'
  csrr t0, mhartid
  add a3,a3,t0
	sw a1,0(a0)
	sw a2,0(a0)
	sw a3,0(a0)
	sw a4,0(a0)
	# j _start
  li  x1, 0
  li  x2, 0
  li  x3, 0
  li  x4, 0
  li  x5, 0
  li  x6, 0
  li  x7, 0
  li  x8, 0
  li  x9, 0
  li  x10,0
  li  x11,0
  li  x12,0
  li  x13,0
  li  x14,0
  li  x15,0
  li  x16,0
  li  x17,0
  li  x18,0
  li  x19,0
  li  x20,0
  li  x21,0
  li  x22,0
  li  x23,0
  li  x24,0
  li  x25,0
  li  x26,0
  li  x27,0
  li  x28,0
  li  x29,0
  li  x30,0
  li  x31,0

  csrw SRC_XIE, 0                   # clear Interrupt Registers
  csrw SRC_XIP, 0

  la t0, trap_entry
  csrw SRC_XTVEC, t0                # set Trap Vector Base Address Register

  /* set to disable FPU */
  li t0, XSTATUS_FS                 # close fpu
  csrc SRC_XSTATUS, t0

.option push
.option norelax
  la gp, __global_pointer$
.option pop

  # park harts with id != 0
  csrr a0, mhartid                  # t0 <- hartid
  slli a1, a0, 2                    # 0 -> 0, 1 -> 4, 2 -> 8 ...
  slli a2, a0, 20                   # shift 1MB

  # shift sp
  la   sp, __stack_start__
  add sp, sp, a2; # sp = sp + a2  
  li   t0, __STACKSIZE__
  # sub t0, t0, a2 # t0 = t0 - a2
  add  sp, sp, t0
  csrw SRC_XSCRATCH, sp

  li t0, SMP_FINISH_FLAG
  add t0, t0, a1
  sw zero, 0(t0)
  .word 0x2d00f

  li t0, SMP_START_FLAG
  add t0, t0, a1                    # get start flag place
  sw a0, 0(t0)                      # write its own hart id!
  .word 0x2d00f

  # then jump to park for subharts
  bnez a0, park                     # if we're not on the hart 0

  jal coremark_main

park:
    # note that a0 now is hart id, a1 is shifted hart id, t0 is flag
    # first flush cache
    .word 0x2d00f
    lw t1, 0(t0)
    # it is not zero, then loop
    bnez t1, park

    li s3,0x54000000
    addi a1,zero,'P'
    addi a2,zero,'S'
    addi a3,zero,'0'
    addi a4,zero,'\n'
    csrr t0, mhartid
    add a3,a3,t0
    sw a1,0(s3)
    sw a2,0(s3)
    sw a3,0(s3)
    sw a4,0(s3)
    
    # should start
    li t0, SMP_PARAM_ADDR
    add t0, t0, a1
    lw a0, 0(t0)                 # load into a0, that is ABI-defined caller
    jal iterate
cores_done:
  wfi
  j cores_done

.align 8
trap_entry:
  addi sp, sp, -272

  SREG x1, 1*REGBYTES(sp)
  SREG x2, 2*REGBYTES(sp)
  SREG x3, 3*REGBYTES(sp)
  SREG x4, 4*REGBYTES(sp)
  SREG x5, 5*REGBYTES(sp)
  SREG x6, 6*REGBYTES(sp)
  SREG x7, 7*REGBYTES(sp)
  SREG x8, 8*REGBYTES(sp)
  SREG x9, 9*REGBYTES(sp)
  SREG x10, 10*REGBYTES(sp)
  SREG x11, 11*REGBYTES(sp)
  SREG x12, 12*REGBYTES(sp)
  SREG x13, 13*REGBYTES(sp)
  SREG x14, 14*REGBYTES(sp)
  SREG x15, 15*REGBYTES(sp)
  SREG x16, 16*REGBYTES(sp)
  SREG x17, 17*REGBYTES(sp)
  SREG x18, 18*REGBYTES(sp)
  SREG x19, 19*REGBYTES(sp)
  SREG x20, 20*REGBYTES(sp)
  SREG x21, 21*REGBYTES(sp)
  SREG x22, 22*REGBYTES(sp)
  SREG x23, 23*REGBYTES(sp)
  SREG x24, 24*REGBYTES(sp)
  SREG x25, 25*REGBYTES(sp)
  SREG x26, 26*REGBYTES(sp)
  SREG x27, 27*REGBYTES(sp)
  SREG x28, 28*REGBYTES(sp)
  SREG x29, 29*REGBYTES(sp)
  SREG x30, 30*REGBYTES(sp)
  SREG x31, 31*REGBYTES(sp)

  li a0,0x54000000
  li a1,'T'
  li a2,'0'
  csrr t0, mhartid
  add a2,a2,t0
	sw a1,0(a0)
	sw a2,0(a0)

  csrr a0, mcause
  csrr a1, mepc
  mv a2, sp
  jal handle_trap
  csrw mepc, a0

  # Remain in M-mode after eret
  li t0, MSTATUS_MPP
  csrs mstatus, t0

  LREG x1, 1*REGBYTES(sp)
  LREG x2, 2*REGBYTES(sp)
  LREG x3, 3*REGBYTES(sp)
  LREG x4, 4*REGBYTES(sp)
  LREG x5, 5*REGBYTES(sp)
  LREG x6, 6*REGBYTES(sp)
  LREG x7, 7*REGBYTES(sp)
  LREG x8, 8*REGBYTES(sp)
  LREG x9, 9*REGBYTES(sp)
  LREG x10, 10*REGBYTES(sp)
  LREG x11, 11*REGBYTES(sp)
  LREG x12, 12*REGBYTES(sp)
  LREG x13, 13*REGBYTES(sp)
  LREG x14, 14*REGBYTES(sp)
  LREG x15, 15*REGBYTES(sp)
  LREG x16, 16*REGBYTES(sp)
  LREG x17, 17*REGBYTES(sp)
  LREG x18, 18*REGBYTES(sp)
  LREG x19, 19*REGBYTES(sp)
  LREG x20, 20*REGBYTES(sp)
  LREG x21, 21*REGBYTES(sp)
  LREG x22, 22*REGBYTES(sp)
  LREG x23, 23*REGBYTES(sp)
  LREG x24, 24*REGBYTES(sp)
  LREG x25, 25*REGBYTES(sp)
  LREG x26, 26*REGBYTES(sp)
  LREG x27, 27*REGBYTES(sp)
  LREG x28, 28*REGBYTES(sp)
  LREG x29, 29*REGBYTES(sp)
  LREG x30, 30*REGBYTES(sp)
  LREG x31, 31*REGBYTES(sp)

  addi sp, sp, 272
  mret