#!/bin/bash

set -e

BASEDIR=$PWD
CM_FOLDER=coremark

cd $BASEDIR/$CM_FOLDER

# run the compile
echo "Start compilation"
make PORT_DIR=../riscv64 compile
riscv64-unknown-elf-objcopy -O binary coremark.riscv coremark.bin
mv coremark.riscv ../coremark.riscv64
mv coremark.bin ../coremark.bin64

make PORT_DIR=../riscv64-baremetal compile
riscv64-unknown-elf-objcopy -O binary coremark.bare.riscv coremark.bare.bin
mv coremark.bare.riscv ../coremark.bare.riscv64
mv coremark.bare.bin ../coremark.bare.bin64

make PORT_DIR=../riscv32 compile
riscv32-unknown-elf-objcopy -O binary coremark.riscv coremark.bin
mv coremark.riscv ../
mv coremark.bin ../

make PORT_DIR=../riscv32-baremetal compile
riscv32-unknown-elf-objcopy -O binary coremark.bare.riscv coremark.bare.bin
mv coremark.bare.riscv ../
mv coremark.bare.bin ../