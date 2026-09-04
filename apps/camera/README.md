# Camera Init — 摄像头初始化程序

GC2083 摄像头初始化 + RTSP 推流 + 拍照，对标官方 `sample_vi_fd` 的流水线。

---

## 编译及部署

```bash
# 编译
make camera_init

# 部署到板子
make deploy
```

---

## 运行时用法

```bash
# 启动 (默认 H.264)
./camera_init

# 指定编码格式
./camera_init h265

# 系统复位 (清理 VPSS/VENC/SYS 状态)
./camera_init reset
```

启动后：
- `rtsp://<板子IP>/h264` — 实时流
- 按 `Enter` — 拍一张 YUV 快照 (capture_xxx.yuv)
- 输入 `stop` 或 `Ctrl+C` — 退出

---

## 流水线架构

```
Sensor (GC2083, I2C)
    │  RAW (1920×1080 BGGR)
    ▼
MIPI RX (2-lane)
    │
    ▼
VI Dev → VI Pipe → ISP (AE + AWB + BIN tuning)
    │  YUV NV21 (1920×1080)
    ▼
VPSS Grp0
    ├── CHN0: NV21   (拍照通道, 1920×1080)
    └── CHN1: YUV420 (推流通道, 1920×1080)
            │
            ▼
         VENC (H.264/H.265)
            │
            ▼
         RTSP Server
```

**模式**：VI 离线 + VPSS 离线（数据经 DDR，对应 SDK `VI_OFFLINE_VPSS_OFFLINE`）

---

## 初始化流程 (12 步)

流程严格对标 `sample_vio.c` → `SAMPLE_COMM_VI_CreateIsp()` → `SAMPLE_TDL_Init_WM()`：

```
SYS_Init → SYS_VI_Open
    │
Step1:  VB 池 (公共 + 4 个 VPSS 专用池)
Step2:  MIPI RX 初始化 (复位、时钟、lane 配置)
Step3:  Sensor 初始化 (SetInit → BusInfo → RegisterCallback → ExpSensorCb → ImageMode → WDRMode)
Step4:  VI Dev (SetDevAttr → EnableDev → BindPipe)
Step5:  VI Pipe (CreatePipe → StartPipe)
Step6:  ISP (AE/AWB Register → SetBindAttr → MemInit → SetPubAttr → ISP_Init → BIN Load → VI Chn Setup → ISP_Run thread)
Step7:  VPSS (CreateGrp → ResetGrp → SetChnAttr×2 → EnableChn×2)
Step8:  VPSS StartGrp
Step9:  Bind (VI Pipe → VPSS Grp0)
Step10: VENC (CreateChn → Start → Bind VPSS CHN1 → VENC)
Step11: RTSP (Create → Start)
```

### 关键流程细节

| 步骤 | 关键点 |
|------|--------|
| **Step3 Sensor** | `pfnRegisterCallback` 必须在 `SetBusInfo` 之后调用，这样 AE/AWB 才能通过 I2C 控制 sensor 寄存器。同时调用 `pfnExpSensorCb` 获取曝光函数指针，分别调用 `set_image_mode` 和 `set_wdr_mode` |
| **Step6 ISP** | AE/AWB 注册顺序：`CVI_AE_Register` → `CVI_AWB_Register` → `CVI_ISP_SetBindAttr`，此顺序与 `SAMPLE_COMM_ISP_Aelib_Callback` 一致。VI Channel 必须在 `CVI_ISP_Run` 之前设置完成 |
| **Step6 BIN** | `CVI_BIN_GetBinName` → 读取文件 → `CVI_BIN_ImportBinData` 加载 **ISP 调优参数**（AE/AWB/CCM/Gamma/NR）。对标 `SAMPLE_COMM_BIN_ReadParaFrombin()`。没有这一步 AE/AWB 用默认值 → 过曝 + 抖动 |
| **Step7 VPSS** | `CVI_VPSS_StartGrp` 必须在 `CVI_SYS_Bind` **之前**调用，SDK 要求在绑定点对点链路之前 Group 已经启动 |
| **Step6 ISP Stop** | `CVI_ISP_Exit` 必须在线程 join 之前调用，线程内的 `CVI_ISP_Run` 是阻塞调用 |

---

## ISP BIN 调优文件

**这是画质与官方程序对齐的关键**。BIN 文件包含：

| 参数类别 | 内容 |
|----------|------|
| AE 曝光表 | 针对 GC2083 的曝光策略、收敛速度、曝光时间/增益范围 |
| AWB 白平衡 | 色温校准数据、色彩矩阵 |
| CCM | 颜色校正矩阵 (Color Correction Matrix) |
| Gamma | 亮度响应曲线 |
| NR | 降噪强度与阈值 |

加载流程：
1. `CVI_BIN_GetBinName()` — 获取 bin 文件路径（通常 `/mnt/data/sensor_cfg.ini` 中配置）
2. `fopen/fread` — 读取整个文件到内存
3. `CVI_BIN_ImportBinData()` — 导入到 ISP 3A 库

加载失败不阻塞启动，会打印警告并使用默认参数。

---

## 链接库

```makefile
FULL_LIBS := -lsys \
    -Wl,--start-group -Wl,-Bstatic \
    -lisp -lisp_algo -lvi -lae -lawb -laf -lsns_gc2083 \
    -lcvi_bin -lcvi_bin_isp -lini -latomic \
    -Wl,--end-group -Wl,-Bdynamic \
    -lm -lpthread -lvpss -lvo -lvenc -lcvi_rtsp -lstdc++ -shared-libgcc
```

新增库（相比之前版本）：
- `-lcvi_bin` / `-lcvi_bin_isp` — ISP BIN 文件加载
- `-lini` — INI 解析（BIN 库依赖）
- `-laf` — 自动对焦（BIN 库依赖 `CVI_ISP_GetAFAttr`）

---

## 画质对齐官方程序的要点

| 项目 | 状态 |
|------|------|
| ISP BIN 调优文件加载 | ✅ `step6_isp.c` 中实现 |
| AE/AWB 注册 (CVI_AE_Register / CVI_AWB_Register) | ✅ |
| Sensor 回调注册 (pfnRegisterCallback) | ✅ |
| Sensor 曝光回调 (pfnExpSensorCb → set_image_mode / set_wdr_mode) | ✅ |
| Sensor I2C 总线配置 (pfnSetBusInfo) | ✅ |
| ISP 发布属性 (SetPubAttr: 1920×1080, 30fps, BAYER_RGGB, WDR_NONE) | ✅ |
| VI Channel 属性 (NV21, SDR8, Linear) | ✅ |
| 缺少 BIN 文件的症状 | 画面过曝、AE 振荡抖动、白平衡不准 |

---

## 文件结构

```
camera/
├── camera_init.c        # 主程序 (12 步流水线 + RTSP 推流 + 交互拍照)
├── Makefile
├── README.md
├── include/
│   ├── step0_reset.h    # 系统复位
│   ├── step1_vb.h       # VB 缓冲池
│   ├── step2_mipi.h     # MIPI RX
│   ├── step3_sensor.h   # Sensor (GC2083)
│   ├── step4_vi.h       # VI Dev
│   ├── step5_pipe.h     # VI Pipe
│   ├── step6_isp.h      # ISP (AE/AWB/BIN)
│   ├── step7_vpss.h     # VPSS
│   ├── step8_bind.h     # VI→VPSS Bind
│   ├── step9_capture.h  # 拍照 (VPSS CHN0)
│   ├── step10_venc.h    # VENC 编码器
│   └── step11_rtsp.h    # RTSP 推流
└── src/
    ├── step0_reset.c
    ├── step1_vb.c
    ├── step2_mipi.c
    ├── step3_sensor.c
    ├── step4_vi.c
    ├── step5_pipe.c
    ├── step6_isp.c
    ├── step7_vpss.c
    ├── step8_bind.c
    ├── step9_capture.c
    ├── step10_venc.c
    └── step11_rtsp.c
```

---

## 参考 SDK 源码

- `cvi_mpi/sample/vio/sample_vio.c` — VI+VPSS+VO 完整流程
- `cvi_mpi/sample/common/sample_common_vi.c` — `SAMPLE_COMM_VI_CreateIsp` / `StartViChn`
- `cvi_mpi/sample/common/sample_common_isp.c` — `SAMPLE_COMM_ISP_Aelib_Callback` / `SAMPLE_COMM_ISP_Run`
- `cvi_mpi/sample/common/sample_common_bin.c` — `SAMPLE_COMM_BIN_ReadParaFrombin`
- `tdl_sdk/sample_video/sample_vi_fd.c` — 官方人脸检测 + RTSP 程序
- `tdl_sdk/sample_video/middleware_utils.c` — `SAMPLE_TDL_Init_WM`
