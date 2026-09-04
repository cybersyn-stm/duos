# ============================================================
# yolo_img_detect Makefile - 6-class image-based YOLO detection
# Usage: ./img_detect_6cls <input.jpg> [model.cvimodel] [conf]
# ============================================================
CC       := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
SYSROOT  := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot
COMMON   := --sysroot=$(SYSROOT)

SDK_ROOT := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2
SDK_INC  := $(SDK_ROOT)/cvi_mpi/include
SDK_LIB  := $(SDK_ROOT)/cvi_mpi/lib
LIB_3RD  := $(SDK_LIB)/3rd

TDL_ROOT := $(SDK_ROOT)/tdl_sdk
TDL_INC  := $(TDL_ROOT)/include
TDL_LIB  := $(TDL_ROOT)/install/lib
TDL_3RD  := $(TDL_ROOT)/install/sample/3rd

LOCAL_INC := include

CPPFLAGS := -I$(SDK_INC) -I$(SDK_INC)/isp/cv181x/ -I$(SDK_INC)/linux/
CPPFLAGS += -I$(TDL_INC) -I$(LOCAL_INC) -D__CV181X__

CFLAGS   := -O2 -Wall -Wextra -Wno-sign-compare $(COMMON)
TPU_LIB  := $(SDK_ROOT)/install/soc_sg2000_milkv_duos_musl_riscv64_sd/tpu_musl_riscv64/cvitek_tpu_sdk/lib
ROOTFS_LIB := $(SDK_ROOT)/install/soc_sg2000_milkv_duos_musl_riscv64_sd/rootfs/mnt/system/lib
LDFLAGS  := $(COMMON) -L$(SDK_LIB) -L$(LIB_3RD) -L$(TDL_LIB) -L$(TDL_3RD)/opencv/lib -L$(TPU_LIB) -L$(ROOTFS_LIB)

FULL_LIBS := -lsys \
    -Wl,--start-group -Wl,-Bstatic \
    -latomic \
    -Wl,--end-group -Wl,-Bdynamic \
    -lm -lpthread -lvo -lvpss -lstdc++ -shared-libgcc \
    -lcvi_tdl -lcvi_ive \
    -lcvikernel -lcvimath -lcviruntime \
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lz

SRCS := src/img_detect_6cls.c src/step1_vb.c
OBJS := $(notdir $(SRCS:.c=.o))

img_detect_6cls: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(FULL_LIBS)
	rm -f $(OBJS)

%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: clean scp deploy

clean:
	rm -f img_detect_6cls *.o

scp:
	scp img_detect_6cls root@192.168.42.1:/root/

deploy-model:
	scp gesture_fixed_int8_sym.cvimodel root@192.168.42.1:/root/

deploy: scp
	scp $(TDL_LIB)/libcvi_tdl.so root@192.168.42.1:/mnt/system/usr/lib/
