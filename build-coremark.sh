#!/bin/bash

set -e

BASEDIR=$PWD
CM_FOLDER=coremark

cd $BASEDIR/$CM_FOLDER

# run the compile
echo "Start compilation"
# make PORT_DIR=../riscv64 compile
# mv coremark.riscv ../

# make PORT_DIR=../riscv64-baremetal compile
# mv coremark.bare.riscv ../

make PORT_DIR=../riscv32-baremetal compile
riscv32-unknown-elf-objcopy -O binary coremark.bare.riscv coremark.bare.bin
mv coremark.bare.riscv ../
mv coremark.bare.bin ../