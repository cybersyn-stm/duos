# ============================================================
# YOLO Gesture Makefile (reuses camera step sources)
# ============================================================
CC       := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
SYSROOT  := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot
COMMON   := --sysroot=$(SYSROOT)

SDK_ROOT := /home/cybersyn/source_sdk/duo-buildroot-sdk-v2
SDK_INC  := $(SDK_ROOT)/cvi_mpi/include
RTSP_INC := $(SDK_ROOT)/cvi_rtsp/include
SDK_LIB  := $(SDK_ROOT)/cvi_mpi/lib
RTSP_LIB := $(SDK_ROOT)/cvi_rtsp/src
LIB_3RD  := $(SDK_LIB)/3rd

TDL_ROOT := $(SDK_ROOT)/tdl_sdk
TDL_INC  := $(TDL_ROOT)/include
TDL_LIB  := $(TDL_ROOT)/install/lib
TDL_3RD  := $(TDL_ROOT)/install/sample/3rd

LOCAL_INC := include

CPPFLAGS := -I$(SDK_INC) -I$(SDK_INC)/isp/cv181x/ -I$(SDK_INC)/linux/
CPPFLAGS += -I$(RTSP_INC) -I$(TDL_INC) -I$(TDL_ROOT)/install/include/cvi_draw_rect -I$(LOCAL_INC)
CPPFLAGS += -D__CV181X__

CFLAGS   := -O2 -Wall -Wextra -Wno-sign-compare $(COMMON)
TPU_LIB  := $(SDK_ROOT)/install/soc_sg2000_milkv_duos_musl_riscv64_sd/tpu_musl_riscv64/cvitek_tpu_sdk/lib
ROOTFS_LIB := $(SDK_ROOT)/install/soc_sg2000_milkv_duos_musl_riscv64_sd/rootfs/mnt/system/lib
LDFLAGS  := $(COMMON) -L$(SDK_LIB) -L$(LIB_3RD) -L$(RTSP_LIB) -L$(TDL_LIB) -L$(TDL_3RD)/opencv/lib -L$(TPU_LIB) -L$(ROOTFS_LIB)

# Local step sources
STEPS_SRC := src/step0_reset.c   \
             src/step1_vb.c      \
             src/step2_mipi.c    \
             src/step3_sensor.c  \
             src/step4_vi.c      \
             src/step5_pipe.c    \
             src/step6_isp.c     \
             src/step7_vpss.c    \
             src/step8_bind.c    \
             src/step10_venc.c   \
             src/step11_rtsp.c

# Compile each step to .o in current dir
STEPS_OBJ := $(notdir $(STEPS_SRC:.c=.o))

FULL_LIBS := -lsys \
    -Wl,--start-group -Wl,-Bstatic \
    -lisp -lisp_algo -lvi -lae -lawb -laf -lsns_gc2083 \
    -lcvi_bin -lcvi_bin_isp -lini -latomic \
    -Wl,--end-group -Wl,-Bdynamic \
    -lm -lpthread -lvpss -lvo -lvenc -lcvi_rtsp -lstdc++ -shared-libgcc \
    -lcvi_tdl -lcvi_ive -Wl,-Bstatic -lcvi_draw_rect -Wl,-Bdynamic \
    -lcvikernel -lcvimath -lcviruntime \
    -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lz

yolo_gesture: $(STEPS_OBJ) yolo_gesture.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(STEPS_OBJ) yolo_gesture.o $(FULL_LIBS)
	rm -f $(STEPS_OBJ) yolo_gesture.o

# Step sources: compile src/stepX.c -> stepX.o (in current dir)
$(STEPS_OBJ): %.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

yolo_gesture.o: src/yolo_gesture.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# ---- Standalone TDL test (jpg/nv21 image -> detect) ----
tdl_test: src/tdl_test.c src/step1_vb.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ src/tdl_test.c src/step1_vb.c $(FULL_LIBS)

.PHONY: clean scp deploy-model deploy tdl_test

clean:
	rm -f yolo_gesture tdl_test *.o

scp:
	scp yolo_gesture root@192.168.42.1:/root/

deploy-model:
	scp gesture_fixed_int8_sym.cvimodel root@192.168.42.1:/root/

deploy: scp deploy-model
	scp $(RTSP_LIB)/libcvi_rtsp.so root@192.168.42.1:/mnt/system/usr/lib/
	scp $(TDL_LIB)/libcvi_tdl.so root@192.168.42.1:/mnt/system/usr/lib/
