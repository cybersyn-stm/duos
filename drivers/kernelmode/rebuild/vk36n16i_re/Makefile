# Minimal reusable Kbuild Makefile template (RISC-V, cross-compile, bear-friendly)
#
# Usage:
#   make
#   make clean
#   bear -- make                # generate compile_commands.json
#   DISABLE_SPECIAL_FLAGS=1 bear -- make   # strip problematic flags for clangd/bear, only for that run

export ARCH ?= riscv
export CROSS_COMPILE ?= /home/cybersyn/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-

KDIR ?= /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/build/sg2000_milkv_duos_musl_riscv64_sd
PWD  := $(shell pwd)

# Module(s)
obj-m += vk36n16i.o

vk36n16i-y := src/vk36n16i.o src/bash_exe.o src/vk36n16i_i2c.o src/vk36n16i_misc.o

ccflags-y +=  -I$(src)/include
# --------------------------------------------------------------------
# Optional: strip flags that may confuse bear/clangd.
# --------------------------------------------------------------------
BAD_FLAGS := -mno-ldd -fno-allow-store-data-races -fconserve-stack
CFLAGS             := $(filter-out $(BAD_FLAGS),$(CFLAGS))
KBUILD_CFLAGS      := $(filter-out $(BAD_FLAGS),$(KBUILD_CFLAGS))
KBUILD_USERCFLAGS  := $(filter-out $(BAD_FLAGS),$(KBUILD_USERCFLAGS))
export CFLAGS KBUILD_CFLAGS KBUILD_USERCFLAGS

# Optional deploy (override as needed)
DEPLOY ?= 0
TARGET ?= root@192.168.42.1:/root/

all:
	@$(MAKE) -C $(KDIR) M=$(PWD) modules
	@rm Module.symvers
	@rm modules.order
	@rm .*.order.cmd
	@rm .*.mod.cmd
	@rm .*.mod.o.cmd
	@rm .*.o.cmd
	@rm .*.ko.cmd
	@rm .*.*.cmd
	@mkdir -p ./out
	@mv *.mod ./out
	@mv *.mod.o ./out
	@mv *.mod.c ./out
	@mv *.o ./out
	@scp *.ko root@192.168.42.1:/root/

ifeq ($(DEPLOY),1)
	@scp src/vk36n16i.ko $(TARGET)
endif

clean:
	@rm -f *.ko
	@rm -f *.c
	@rm -f src/*.mod.c src/*.o src/*.ko src/*mod
	@rm -f Module.symvers
	@rm -f modules.orderrm -f *.ko
	@rm -rf ./out
