# ============================================================
# Camera 学习工程 Makefile（逐步骤构建）
# ============================================================
CC       := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
SYSROOT  := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot
COMMON   := --sysroot=$(SYSROOT)

SDK_INC  := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/cvi_mpi/include
RTSP_INC := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/cvi_rtsp/include
SDK_LIB  := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/cvi_mpi/lib
RTSP_LIB := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/cvi_rtsp/src
LIB_3RD  := $(SDK_LIB)/3rd

LOCAL_INC := include

CPPFLAGS := -I$(SDK_INC)
CPPFLAGS += -I$(SDK_INC)/isp/cv181x/
CPPFLAGS += -I$(SDK_INC)/linux/
CPPFLAGS += -I$(RTSP_INC)
CPPFLAGS += -I$(LOCAL_INC)
CPPFLAGS += -D__CV181X__

CFLAGS   := -O2 -Wall -Wextra -Wno-sign-compare $(COMMON)
LDFLAGS  := $(COMMON) -L$(SDK_LIB) -L$(LIB_3RD) -L$(RTSP_LIB)

# 链接库（随步骤逐步追加）
LDLIBS   := -lsys -latomic -lpthread -lm

# 统一链接库
FULL_LIBS := -lsys -Wl,--start-group -Wl,-Bstatic -lisp -lisp_algo -lvi -lae -lawb -laf -lsns_gc2083 -lcvi_bin -lcvi_bin_isp -lini -latomic -Wl,--end-group -Wl,-Bdynamic -lm -lpthread -lvpss -lvo -lvenc -lcvi_rtsp -lstdc++ -shared-libgcc

# ------------------------------------------------------------
# 逐步骤独立测试目标（旧结构，保持不变）
# ------------------------------------------------------------
STEPS := step0_reset    \
         step1_vb      \
         step2_mipi    \
         step3_sensor  \
         step4_vi      \
         step5_pipe    \
         step6_isp     \
         step7_vpss    \
         step8_bind    \
         step9_capture

step0_reset:  LDLIBS := -lsys -latomic -lpthread -lm
step1_vb:      LDLIBS := -lsys -latomic -lpthread -lm
step2_mipi:    LDLIBS := $(FULL_LIBS)
step3_sensor:  LDLIBS := $(FULL_LIBS)
step4_vi:      LDLIBS := $(FULL_LIBS)
step5_pipe:    LDLIBS := $(FULL_LIBS)
step6_isp:     LDLIBS := $(FULL_LIBS)
step7_vpss:    LDLIBS := $(FULL_LIBS) -lvpss
step8_bind:    LDLIBS := $(FULL_LIBS) -lvpss
step9_capture: LDLIBS := $(FULL_LIBS) -lvpss

# ------------------------------------------------------------
# 完整 camera_init（新结构：src/*.c + camera_init.c）
# ------------------------------------------------------------
CAMERA_SRCS := src/step1_vb.c src/step2_mipi.c src/step3_sensor.c src/step4_vi.c src/step5_pipe.c camera_init.c
CAMERA_SRCS += src/step6_isp.c src/step7_vpss.c src/step8_bind.c src/step9_capture.c
CAMERA_SRCS += src/step10_venc.c src/step11_rtsp.c
CAMERA_SRCS += src/step0_reset.c
CAMERA_OBJS := $(CAMERA_SRCS:.c=.o)

camera_init: $(CAMERA_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(CAMERA_OBJS) $(FULL_LIBS)
	rm -f $(CAMERA_OBJS)

# standalone targets（无根目录 .c，从 src/ 编译）
step0_reset: src/step0_reset.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

step4_vi: src/step4_vi.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

step5_pipe: src/step5_pipe.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

step6_isp: src/step6_isp.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

step7_vpss: src/step7_vpss.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lvpss

step8_bind: src/step8_bind.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lvpss

step9_capture: src/step9_capture.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lvpss

.PHONY: all clean steps deploy deploy-libs
all: camera_init $(STEPS)

steps:
	@echo "Steps: $(STEPS)"

scp:
	scp camera_init root@192.168.42.1:/root/

deploy-libs:
	scp $(RTSP_LIB)/libcvi_rtsp.so root@192.168.42.1:/mnt/system/usr/lib/
	@echo "Library deployed. Set LD_LIBRARY_PATH on device:"
	@echo "  export LD_LIBRARY_PATH=/mnt/system/lib:/mnt/system/usr/lib:/mnt/system/usr/lib/3rd"

deploy: scp deploy-libs
# 通用编译规则
%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(STEPS) camera_init *.o src/*.o
