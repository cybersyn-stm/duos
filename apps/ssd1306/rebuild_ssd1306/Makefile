# User-mode Makefile (cross compile + bear/clangd friendly)

CC := /home/cybersyn/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
SYSROOT := /home/cybersyn/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot

APP  ?= menu

SRCS :=	$(shell find ./src/*c) 
OBJS := $(patsubst ./src/%.c, obj/%.o, $(SRCS))

COMMON := --sysroot=$(SYSROOT)

CPPFLAGS ?=
CPPFLAGS += -I./include/               # so "ssd1306_8x16.h" can be found

CFLAGS  ?= -O2 -Wall -Wextra $(COMMON)
LDFLAGS ?= $(COMMON)
LDLIBS  ?=

all: $(APP)
	scp menu root@192.168.42.1:/root/

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

obj/%.o: ./src/%.c
	mkdir -p obj
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
debug:
	@echo $(SRCS)

clean:
	rm -f $(APP) $(OBJS)
	rm -rf obj
