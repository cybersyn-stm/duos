# ============================================================
# Milk-V Duo Camera + RTSP Makefile
# ============================================================

# 交叉编译器
CC      := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
SYSROOT := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot

# SDK 路径
SDK     := /home/cybersyn/sdk/duo-buildroot-sdk-v2
MPI_INC := $(SDK)/cvi_mpi/include
RTSP_INC:= $(SDK)/cvi_rtsp/include
MPI_LIB := $(SDK)/cvi_mpi/lib
RTSP_LIB:= $(SDK)/cvi_rtsp
COMMON  := --sysroot=$(SYSROOT)

# 输出
APP := camera_init

# 源文件
SRCS := camera_init.c \
        src/step7_vpss.c \
        src/step10_venc.c \
        src/step11_rtsp.c
OBJS := $(SRCS:.c=.o)

# 头文件路径
CPPFLAGS := -I$(MPI_INC) \
            -I$(MPI_INC)/isp/cv181x/ \
            -I$(MPI_INC)/linux/ \
            -I$(RTSP_INC) \
            -Iinclude \
            -D__CV181X__

# 编译选项: -Wno-sign-compare 避免 SDK 头文件里的 sign 警告
CFLAGS  := -O2 -Wall -Wextra -Wno-sign-compare $(COMMON)

# 链接
LDFLAGS := $(COMMON)
LDFLAGS += -L$(MPI_LIB) \
           -L$(MPI_LIB)/3rd/ \
           -L$(RTSP_LIB)/src/

LDLIBS  := -lvi -lvpss -lvo -lisp -lisp_algo -lae -lawb \
           -lsns_gc2083 -lmipi_tx -lsys -lmisc \
           -lvenc -lcvi_rtsp -lstdc++ \
           -lgcc_s -lpthread -lm -latomic

# ─── 目标 ───
all: $(APP)
	scp $^ root@192.168.42.1:/root/
	@echo "部署完成，在设备上运行: ./$(APP)"

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(APP) $(OBJS)
	rm -rf photo/

# 单独部署
deploy: $(APP)
	scp $^ root@192.168.42.1:/root/
	@echo "部署完成"

# 部署 RTSP .so 库（首次需要）
deploy-libs:
	scp $(RTSP_LIB)/src/libcvi_rtsp.so root@192.168.42.1:/mnt/system/usr/lib/
	@echo "RTSP 库部署完成"
