# Milk-V Duo SDK —— 板子 overlay 机制说明

> 说明：本文记录 `duo-buildroot-sdk-v2` 中，rootfs 自定义文件（overlay）是如何被拷贝进最终镜像的。
> 适用场景：想往根文件系统里加文件（脚本、配置、网页、ko 模块、二进制）时，判断该把文件放哪个目录、以及各目录的优先级。

---

## 1. 核心概念

**overlay = 覆盖层**。你放在 overlay 目录里的文件，会在打包 rootfs 时被"原样复制"进根文件系统的对应位置。

- `overlay 里的 etc/hostapd.conf` → 镜像里的 `/etc/hostapd.conf`
- `overlay 里的 usr/local/bin/webapp.py` → 镜像里的 `/usr/local/bin/webapp.py`
- `overlay 里的 www/index.html` → 镜像里的 `/www/index.html`

目录结构直接映射为 rootfs 的目录结构，**放哪就是哪**。

---

## 2. 关键变量

打包时由 `build/envsetup_milkv.sh` 定义，`build/Makefile` 使用：

| 变量 | 值（以 Duo256M musl 为例） | 含义 |
|------|---------------------------|------|
| `TOP_DIR` | `/home/cybersyn/sdk/duo-buildroot-sdk-v2` | SDK 根目录 |
| `MV_BOARD` | `milkv-duo256m-musl-riscv64-sd` | 板子目录名 |
| `MV_VENDOR` | `milkv` | 厂商 |
| `SDK_VER` | `musl_riscv64` | 工具链/ABI 版本（musl+riscv64） |
| `BR_BOARD_TYPE` / `MV_BOARD_TYPE` | `duo256m` | 板型：`duo` / `duo256m` / `duos` |
| `BR_ROOTFS_DIR` | `$OUTPUT_DIR/tmp-rootfs` | 临时 staging 根目录 |
| `BR_OVERLAY_DIR` | `buildroot/board/milkv/<MV_BOARD>/overlay` | 最终交给 buildroot 的 overlay |

**`MV_BOARD_TYPE` 的判定规则**（`envsetup_milkv.sh` 约 916 行）：

```sh
if [[ ${MV_BOARD} == *"-duo-"* ]]; then      MV_BOARD_TYPE="duo"
elif [[ ${MV_BOARD} == *"-duo256m-"* ]]; then MV_BOARD_TYPE="duo256m"
elif [[ ${MV_BOARD} == *"-duos-"* ]]; then    MV_BOARD_TYPE="duos"
fi
```

---

## 3. 拷贝流程（最重要）

打包 rootfs 走 `build/Makefile` 的 `br-rootfs-prepare` 目标（约 563～604 行）。
所有 overlay 目录都会被 `cp -rf` 拷进同一个 `BR_ROOTFS_DIR`（`tmp-rootfs`），
**拷完后再整体拷到 `BR_OVERLAY_DIR`，最后交给 buildroot 生成 rootfs**。

按**顺序**执行（后面的会覆盖前面的同名文件）：

```
1. 系统产物      →  tmp-rootfs/mnt/system/       （ko、mmf 库等构建产物）
2. device/generic/br_overlay/common/*           → tmp-rootfs/   （所有板子通用）
3. device/generic/br_overlay/<SDK_VER>/*        → tmp-rootfs/   （musl_riscv64 专用）
4. device/generic/rootfs_overlay/common/*       → tmp-rootfs/   （所有板子通用）
5. device/generic/rootfs_overlay/<SDK_VER>/*    → tmp-rootfs/   （若目录存在）
6. device/generic/rootfs_overlay/<BR_BOARD_TYPE>/* → tmp-rootfs/（duo/duo256m/duos）
7. device/<MV_BOARD>/overlay/*                  → tmp-rootfs/   （单板专用，若存在）
8. strip 处理（.ko / .so* / 可执行文件瘦身）
9. rm -rf BR_OVERLAY_DIR; 把 tmp-rootfs/* 整体拷到 BR_OVERLAY_DIR
```

### 优先级结论

**后面覆盖前面**，优先级从低到高：

```
br_overlay/common  <  br_overlay/<SDK_VER>  <  rootfs_overlay/common
   <  rootfs_overlay/<SDK_VER>  <  rootfs_overlay/<BR_BOARD_TYPE>
   <  device/<MV_BOARD>/overlay
```

即：**单板专用 overlay 优先级最高**，能覆盖通用 overlay 里的同名文件。

---

## 4. 各目录的用途（放哪的判断标准）

| 目录 | 什么时候放这 |
|------|-------------|
| `device/generic/br_overlay/common/` | **所有板子都要的文件**（跨板通用）。如网页、通用脚本、通用配置 |
| `device/generic/br_overlay/<SDK_VER>/` | 只针对某工具链/ABI 的文件（如 `musl_riscv64/` 下的动态链接器） |
| `device/generic/rootfs_overlay/common/` | 通用 rootfs 文件（`mnt/system`、`mnt/cfg` 等分区挂载内容） |
| `device/generic/rootfs_overlay/<BR_BOARD_TYPE>/` | 按**板型**区分：`duo` / `duo256m` / `duos` 各放各的 |
| `device/<MV_BOARD>/overlay/` | **单板专属**、不想影响其它板子的文件。默认不存在，需自行创建 |

### 实战建议

- 文件**只给某一块板子用** → 放 `device/<MV_BOARD>/overlay/`（需 `mkdir` 创建）
- 文件**只给某板型用**（如所有 256M 板） → 放 `rootfs_overlay/<BR_BOARD_TYPE>/`
- 文件**所有板子都要** → 放 `br_overlay/common/`（最省事，但会影响其它板）

---

## 5. 实例：NIXIETUBE 时钟 Web 应用

当前这套文件**全部放在 `device/generic/br_overlay/common/`**（跨板通用），对应关系：

| 源文件（`br_overlay/common/` 下） | rootfs 落点 | 权限 |
|-----------------------------------|-------------|------|
| `usr/local/bin/webapp.py` | `/usr/local/bin/webapp.py` | 755 |
| `www/index.html` | `/www/index.html` | 644 |
| `www/app.js` | `/www/app.js` | 644 |
| `www/style.css` | `/www/style.css` | 644 |
| `etc/hostapd.conf` | `/etc/hostapd.conf` | 644 |
| `etc/init.d/S99xhostapd` | `/etc/init.d/S99xhostapd` | 755 |
| `etc/init.d/S99yweb` | `/etc/init.d/S99yweb` | 755 |

> 注意：
> - 脚本/二进制（`webapp.py`、`S99*`）必须 `chmod 755`，否则开机不会执行。
> - 因为放在 `common/`，换任何板子（Duo / Duo256M / DuoS）都会被打进去。

---

## 6. 改动 overlay 后的打包步骤

改完 overlay 文件后，只需增量打包（无需全量重建）：

```bash
cd /home/cybersyn/sdk/duo-buildroot-sdk-v2
source build/envsetup_milkv.sh milkv-duo256m-musl-riscv64-sd   # 换成你的板子

pack_rootfs      # 重新应用 overlay + 重建 rootfs
pack_sd_image    # 重新生成 SD 卡镜像
```

镜像输出位置：
`install/soc_sg2002_milkv_duo256m_musl_riscv64_sd/milkv-duo256m-musl-riscv64-sd.img`

---

## 7. 验证方法

打包后可直接查看 staging 目录，确认文件已正确落位：

```bash
# overlay 应用后的临时根目录
ls -la install/soc_sg2002_milkv_duo256m_musl_riscv64_sd/tmp-rootfs/
ls -la install/soc_sg2002_milkv_duo256m_musl_riscv64_sd/tmp-rootfs/etc/init.d/
ls -la install/soc_sg2002_milkv_duo256m_musl_riscv64_sd/tmp-rootfs/www/

# 或直接看 buildroot 最终 overlay 目录
ls -la buildroot/board/milkv/milkv-duo256m-musl-riscv64-sd/overlay/
```

---

## 8. 相关源码位置

| 内容 | 位置 |
|------|------|
| overlay 拷贝逻辑 | `build/Makefile` → `br-rootfs-prepare`（约 563～604 行） |
| 变量定义 | `build/envsetup_milkv.sh`（约 916～933 行） |
| 通用 overlay | `device/generic/br_overlay/`、`device/generic/rootfs_overlay/` |
| 单板配置 | `device/<MV_BOARD>/boardconfig.sh`（`MV_BOARD_LINK` 指向 `build/boards/`） |
| 单板 overlay | `device/<MV_BOARD>/overlay/`（默认不存在） |
