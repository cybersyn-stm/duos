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
obj-m += tm1650.o 

tm1650-y += src/tm1650.o src/tm1650_misc.o 

ccflags-y += -I$(src)/include
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
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -f *.ko
