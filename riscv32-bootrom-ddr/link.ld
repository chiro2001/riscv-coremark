/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020/12/12     bernard      The first version
 */

OUTPUT_ARCH( "riscv" )

/*
 * Memory layout:
 * 0x80000000 - end: DDR
 */

ROM_ADDR = 0x10000;
MEM_ADDR = 0x80000000;
MEMORY
{
   ROM (RI)  : ORIGIN = ROM_ADDR, LENGTH = 0x8000
   RAM (RWX) : ORIGIN = MEM_ADDR, LENGTH = 0x8000
}
__STACKSIZE__ = 16384;

ENTRY(_start)
SECTIONS
{
    . = 0x10000 ;

    /* __STACKSIZE__ = 4096; */

    .start :
    {
        *(.start);
    } > ROM

    . = ALIGN(4);

    .text : 
    {
        *(.text)                        /* remaining code */
        *(.text.*)                      /* remaining code */
        *(.rodata)                      /* read-only data (constants) */
        *(.rodata*)
    } > ROM

    .data : 
    {
        *(.data)
        *(.data.*)
    
        *(.data1)
        *(.data1.*)

        . = ALIGN(4);
        PROVIDE( __global_pointer$ = . + 0x800 );

        *(.sdata)
        *(.sdata.*)
    } > RAM

    .stack :
    {
        . = ALIGN(64);
        __stack_start__ = .;

        . += __STACKSIZE__;
        __stack_cpu0 = .;
    } > RAM

    .sbss : 
    {
    __bss_start = .;
        *(.sbss)
        *(.sbss.*)
        *(.dynsbss)
        *(.scommon)
    } > RAM

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.dynbss)
        *(COMMON)
    __bss_end = .;
    } > RAM

    _end = .;
}
