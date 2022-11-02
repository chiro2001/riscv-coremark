base_dir := $(abspath .)

bootrom_target := ../overlay/coremark.bootrom.bin ../overlay/coremark.bootrom.dump ../overlay/coremark.bootrom.riscv

bootrom: $(bootrom_target)

bootrom-build:
	make -C coremark PORT_DIR=$(base_dir)/riscv32-bootrom compile

../overlay/coremark.bootrom.bin: bootrom-build ../overlay/coremark.bootrom.riscv
	riscv32-unknown-elf-objcopy -O binary ../overlay/coremark.bootrom.riscv ../overlay/coremark.bootrom.bin
../overlay/coremark.bootrom.dump: bootrom-build ../overlay/coremark.bootrom.riscv
	riscv32-unknown-elf-objdump -d ../overlay/coremark.bootrom.riscv > ../overlay/coremark.bootrom.dump
../overlay/coremark.bootrom.riscv: bootrom-build
	mv $(base_dir)/coremark/coremark.bootrom.riscv ../overlay

clean:
	-rm -rf ../overlay/*