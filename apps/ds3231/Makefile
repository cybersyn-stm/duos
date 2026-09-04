# ============================================================
# User-mode 通用 Makefile 模板（交叉编译）
# ------------------------------------------------------------
# 用法：
#   1) 按你的 SDK 路径修改 CC / SYSROOT
#   2) 在命令行或上层 Makefile 传入 APP、SRCS（推荐）
#      例如：make APP=hello SRCS="hello.c util.c"
#   3) 执行 make
#      注意：当前 all 目标默认会执行 scp 部署（见下方注释）
# ============================================================

# 交叉编译器路径（按实际环境修改）
CC := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
# sysroot 路径（按实际环境修改）
SYSROOT := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot

# 最终可执行文件名（如 hello）
# 建议通过命令行传入：make APP=hello
APP  ?= ds3231_init

# 源文件列表（如 hello.c util.c）
# 建议通过命令行传入：make SRCS="hello.c util.c"
SRCS := ./src/ds3231_init.c ./src/ssd1306_8x16.c ./src/pomodoro.c
# 自动把 .c 映射成 .o
OBJS := $(SRCS:.c=.o)

# 公共编译/链接参数
COMMON := --sysroot=$(SYSROOT)

# 预处理参数：头文件搜索路径、宏定义等
CPPFLAGS ?=
CPPFLAGS += -I./include/               # so "ssd1306_8x16.h" can be found

# 编译参数（可按需追加 -g / -DXXX 等）
CFLAGS  ?= -O2 -Wall -Wextra $(COMMON)
# 链接参数
LDFLAGS ?= $(COMMON)
# 额外库（如 -lm -lpthread）
LDLIBS  ?=

# 默认目标：
# 1) 先构建 $(APP)
# 2) 再通过 scp 上传到开发板
# 如果你只想本地编译，可注释下一行 scp 或改为单独 deploy 目标。
all: $(APP)
	scp $^ root@192.168.42.1:/root/

# 链接：由所有目标文件生成最终程序
$(APP): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# 编译：每个 .c 生成对应 .o
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# 清理构建产物
clean:
	rm -f $(APP) $(OBJS)
