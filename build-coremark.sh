#!/bin/bash

set -e

BASEDIR=$PWD
CM_FOLDER=coremark

cd $BASEDIR/$CM_FOLDER

# run the compile
echo "Start compilation"
make PORT_DIR=../riscv64 compile
riscv64-unknown-elf-objcopy -O binary coremark.riscv64 coremark.bin64

make PORT_DIR=../riscv64-baremetal compile
riscv64-unknown-elf-objcopy -O binary coremark.bare.riscv64 coremark.bare.bin64

make PORT_DIR=../riscv32 compile
riscv32-unknown-elf-objcopy -O binary coremark.riscv coremark.bin

make PORT_DIR=../riscv32-baremetal compile
riscv32-unknown-elf-objcopy -O binary coremark.bare.riscv coremark.bare.bin

make PORT_DIR=../riscv32-bootrom compile
riscv32-unknown-elf-objcopy -O binary coremark.bootrom.riscv coremark.bootrom.bin
riscv32-unknown-elf-objdump -D coremark.bootrom.riscv > coremark.boortom.dump

mv *.riscv* ../
mv *.bin* ../
mv *.dump ../