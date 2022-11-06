#!/bin/bash

set -e

BASEDIR=$PWD
CM_FOLDER=coremark

cd $BASEDIR/$CM_FOLDER

# run the compile
echo "Start compilation"
make PORT_DIR=../riscv64 compile
riscv64-unknown-elf-objcopy -O binary coremark.riscv64 coremark.bin64
riscv64-unknown-elf-objdump -d -S coremark.riscv64 > coremark64.asm

make PORT_DIR=../riscv64-baremetal compile
riscv64-unknown-elf-objcopy -O binary coremark.bare.riscv64 coremark.bare.bin64
riscv64-unknown-elf-objdump -d -S coremark.bare.riscv64 > coremark.bare64.asm

make PORT_DIR=../riscv32 compile
riscv32-unknown-elf-objcopy -O binary coremark.riscv coremark.bin
riscv32-unknown-elf-objdump -d -S coremark.riscv > coremark.asm

make PORT_DIR=../riscv32-baremetal compile
riscv32-unknown-elf-objcopy -O binary coremark.bare.riscv coremark.bare.bin
riscv32-unknown-elf-objdump -d -S coremark.bare.riscv > coremark.bare.asm

make PORT_DIR=../riscv32-bootrom compile
riscv32-unknown-elf-objcopy -O binary coremark.bootrom.riscv coremark.bootrom.bin
riscv32-unknown-elf-objdump -d -S coremark.bootrom.riscv > coremark.bootrom.asm

mv *.riscv* ../
mv *.bin* ../
# mv *.dump ../
mv *.asm ../