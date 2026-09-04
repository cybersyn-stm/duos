CC := /home/cybersyn/sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
# gpiod 的头文件和库在 buildroot 的 sysroot 里（host-tools 自带 sysroot 是空的）
SYSROOT := /home/cybersyn/sdk/duo-buildroot-sdk-v2/buildroot/output/milkv-duos-musl-riscv64-sd/host/riscv64-buildroot-linux-musl/sysroot

APP  ?= nixietube
SRCS := ./src/nixietube.c
OBJS := $(SRCS:.c=.o)

CPPFLAGS ?=
CPPFLAGS += -I./include/

CFLAGS  ?= -mabi=lp64d -march=rv64imafd -fstack-protector-strong -fPIE -O2 -Wall -Wextra --sysroot=$(SYSROOT)
LDFLAGS ?= --sysroot=$(SYSROOT) -Wl,-z,now -Wl,-z,relro
LDLIBS  ?= -lgpiod

all: $(APP)
	scp $^ root@192.168.42.1:/root/

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(APP) $(OBJS)
