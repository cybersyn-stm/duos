# Rootfs 1000:1000 属主问题：根因与修复

> 适用对象：Milk-V Duo 系列 buildroot SDK
> 修复来源：提交 `6298c5455`（`feat: RTL8188F WiFi support and SD image packaging fixes`）

## 1. 问题现象

SD 镜像烧录后，rootfs 分区的文件属主/属组变成了 `1000:1000`，而不是正确的 `root:root`（`0:0`）。

## 2. 根因链路

问题**不在 buildroot 本身**，而在 **SD 镜像打包阶段**：

1. **buildroot 阶段正确**：buildroot 用 fakeroot 生成 `rootfs.tar.xz`，内部属主为 `0:0`。
2. **`sd_image` 阶段丢失属主**（`build/Makefile`）：
   ```make
   tar xf $(BR_DIR)/output/$(BR_BOARD)/images/rootfs.tar.xz -C $(OUTPUT_DIR)/br-rootfs
   ```
   `tar xf` 由普通用户（uid 1000）执行，tar 归档里的 `0:0` 属主被解压为当前用户 → `br-rootfs/` 变为 `1000:1000`。
3. **genimage 重建 ext4 未纠正属主**：`gen_burn_image_sd.sh` 中 genimage 使用 `use-mke2fs=true`，从 `fs/`（软链到 `br-rootfs/`）重新生成 `rootfs.ext4`。若不加 fakeroot，mke2fs 以普通用户运行，读到的属主就是 `1000:1000`。

## 3. 修复内容

文件：`device/gen_burn_image_sd.sh`（仅改 2 行）

### 改动 1：PATH 增加 buildroot 的 sbin/bin（第 29 行）

```diff
-export PATH=${BR_HOST_BIN}:${PATH}
+export PATH=${BR_HOST_BIN}:${BR_HOST_BIN}/../sbin:${BR_HOST_BIN}/../bin:${PATH}
```

作用：让 genimage 优先调用 buildroot 自带的 `mke2fs`（位于 `host/sbin/`），而非宿主机系统的 mke2fs，避免版本/特性不一致。

### 改动 2：genimage 前加 `fakeroot --`（第 37 行）

```diff
-genimage --config ${TOP_DIR}/device/${MV_BOARD}/genimage.cfg --rootpath fs/ --inputpath ${PWD} --outputpath ${PWD}
+fakeroot -- genimage --config ${TOP_DIR}/device/${MV_BOARD}/genimage.cfg --rootpath fs/ --inputpath ${PWD} --outputpath ${PWD}
```

作用：让 genimage 在 fakeroot（假 root）环境下运行，重建 ext4 时将文件属主正确写为 `0:0`。

## 4. 核心结论

- 1000:1000 由 **SD 打包路径**引入：`tar xf`（丢属主） + `genimage`（无 fakeroot 重建）。
- 核心修复是 **`fakeroot --` 包裹 genimage**，在重新打 ext4 这一步纠正属主。
- PATH 改动是配套修正，确保使用 buildroot 的 mke2fs，两者配合才能完整修复。

## 5. 验证方法

```bash
# 查看 buildroot 产物（应为 0:0，验证第 1 步正确）
tar -tJvf buildroot/output/<board>/images/rootfs.tar.xz --numeric-owner

# 查看 br-rootfs 宿主机属主（应为 1000:1000，验证第 2 步丢属主）
ls -land install/<board>/br-rootfs/

# 查看最终 rootfs.ext4 属主（修复后应为 0:0）
DBG=buildroot/output/<board>/host/sbin/debugfs
$DBG -R "stat /" install/<board>/rootfs.ext4 | grep -E "User:|Group:"
```

预期结果（修复后）：
```
User:     0   Group:     0
```

## 6. 附加说明

- 修复后 `busybox` 属主为 `0755`（而非 source_sdk 中的 `4755` suid），说明 `fakeroot -- genimage` 重打 ext4 后 suid 位未保留，如需 suid 需另行检查 device table 处理。
- 若需修复其他 SDK 副本（如 `source_sdk`），将上述两行改动移植到对应 `device/gen_burn_image_sd.sh` 即可。
