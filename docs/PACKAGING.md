# Nokia N1 Boot Image Packaging Guide

> **⚠️ 注意**: 本文档已于 2026-03-02 更新，反映当前的 **TWRP-based lk2nd** 方案。  
> 之前的 Zig Unikernel 方案已废弃（`src/n1_bootloader` 不再存在），旧版本见 `PACKAGING.md.old`。

## 概述

Nokia N1 使用 Droidboot/Bootstub 作为引导程序链。它期望接收标准 **Android Boot Image (boot.img)** 格式：
1. Boot image magic (`ANDROID!`)
2. 内存加载地址（必须匹配原厂参数）
3. 命令行参数（特别是 `androidboot.hardware=mofd_v1`）
4. Second bootloader (Bootstub) — **必须包含**

当前方案：使用**原厂 3.10 内核 + 修改的 TWRP ramdisk** 作为 "lk2nd" 引导环境，再通过 kexec 加载目标内核。

---

## 快速开始

### 1. 从原始 ramdisk 提取干净副本

> **❗不要**直接从 `ramdisk_unpacked/` 打包，其中可能累积了多余文件导致启动失败。

```bash
cd /tmp && rm -rf clean_ramdisk && mkdir clean_ramdisk && cd clean_ramdisk
zcat /home/drie/n1_dev/twrp_dev/ramdisk | cpio -id

# 添加工具 (仅添加需要的文件)
cp /home/drie/n1_dev/twrp_dev/ramdisk_unpacked/sbin/{strace,devmem2,pmic_tool,n1_display,n1_hwinfo.sh} sbin/
chmod 755 sbin/strace sbin/devmem2 sbin/pmic_tool sbin/n1_display sbin/n1_hwinfo.sh
```

### 2. 打包 ramdisk + boot.img

> **❗必须使用 `fakeroot`**，否则文件属主为当前用户而非 root:root，导致设备卡在 splash screen。

```bash
# 打包 ramdisk (fakeroot 确保 root:root)
cd /tmp/clean_ramdisk
fakeroot -- sh -c 'find . | cpio -H newc -o 2>/dev/null | gzip -9 > /tmp/clean_ramdisk.gz'

# 创建 boot.img
cd /home/drie/n1_dev/twrp_dev
mkbootimg \
    --kernel kernel \
    --ramdisk /tmp/clean_ramdisk.gz \
    --second second \
    --base 0x10000000 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x01000000 \
    --second_offset 0x00f00000 \
    --tags_offset 0x00000100 \
    --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx androidboot.serialno=01234567890123456789 gpt bootboost=1 panic=15" \
    --pagesize 2048 \
    -o boot_final.img
```

**cmdline 说明**:
- `androidboot.spid=xxxx:...` 和 `androidboot.serialno=0123...` 是**占位符**，Bootstub 启动时会替换为真实值
- `panic=15` 使 kernel panic 后 15 秒自动重启

### 3. 刷入 (boot 分区 = 32MB)

```bash
# 方式 A: 通过 ADB (TWRP 运行时)
adb shell dd if=/dev/zero of=/dev/block/by-name/boot bs=4096 count=8192
adb push boot_final.img /tmp/
adb shell dd if=/tmp/boot_final.img of=/dev/block/by-name/boot bs=4096
adb reboot

# 方式 B: 通过 Fastboot
adb reboot bootloader
fastboot flash boot boot_final.img
fastboot reboot
```

---

## Boot Image 布局

```
┌─────────────────────────────────────┐
│  Boot Header (2048 bytes, page 0)   │  <- Magic "ANDROID!"
│  - Kernel/Ramdisk/Second 地址       │
│  - Command line                     │
└─────────────────────────────────────┘
│  Kernel (padded to page boundary)   │  <- 原厂 3.10.62 bzImage (6.3MB)
├─────────────────────────────────────┤
│  Ramdisk (padded to page boundary)  │  <- 修改的 TWRP ramdisk (~8MB)
├─────────────────────────────────────┤
│  Second bootloader                  │  <- 原厂 Bootstub 1.4 (8KB) ← 必须包含!
└─────────────────────────────────────┘
```

---

## 关键参数 (Ground Truth — 从原厂镜像验证)

### 内存地址

这些参数直接从原厂 `droidboot.img` 和 `stock_boot_cnb19.img` 的 header 中提取：

```
BASE_ADDR        = 0x10000000     ← 原厂 boot.img header 中的基址
KERNEL_OFFSET    = 0x00008000     ← Kernel load: 0x10008000
RAMDISK_OFFSET   = 0x01000000     ← Ramdisk load: 0x11000000
SECOND_OFFSET    = 0x00f00000     ← Second load: 0x10f00000
TAGS_ADDR        = 0x10000100
PAGE_SIZE        = 2048
```

**验证来源**: `file images/original/droidboot.img` 输出:
```
Android bootimg, kernel (0x10008000), ramdisk (0x11000000), second stage (0x10f00000), page size: 2048
```

### 命令行参数 (从原厂提取)

```
init=/init pci=noearly loglevel=0 vmalloc=256M
androidboot.hardware=mofd_v1      ← 必须保留，否则硬件初始化被跳过
watchdog.watchdog_thresh=60       ← SCU 看门狗超时
gpt bootboost=1
```

---

## 组件来源

| 组件 | 文件 | 来源 | 可信度 |
|------|------|------|--------|
| kernel | `twrp_dev/kernel` | 从原厂 droidboot.img 提取 | ✅ Ground Truth (SHA256 已验证) |
| second | `twrp_dev/second` | 从原厂 droidboot.img 提取 | ✅ Ground Truth (SHA256 已验证) |
| ramdisk | `twrp_dev/ramdisk` | 原始 TWRP ramdisk (基线) | ✅ Ground Truth |
| 添加工具 | `twrp_dev/ramdisk_unpacked/sbin/` | strace/devmem2/pmic_tool/n1_display/n1_hwinfo.sh | ⚠️ Modified |
| 打包参数 | 本文档 | 从原厂 header 提取 | ✅ Ground Truth |

---

## 我们对 ramdisk 的修改

以下工具添加到原始 TWRP ramdisk 中 (musl-gcc x86_64 静态链接)：

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `sbin/strace` | 新增 | strace 6.12 系统调用跟踪 (1.4MB) |
| `sbin/devmem2` | 新增 | 物理内存/MMIO 读写 (34KB) |
| `sbin/pmic_tool` | 新增 | PMIC 寄存器读写/dump (38KB) |
| `sbin/n1_display` | 新增 | 显示/背光/GPIO 控制 (34KB) |
| `sbin/n1_hwinfo.sh` | 新增 | 硬件信息采集脚本 (5.7KB) |

> 注意: 64-bit kexec 二进制单独存放在 `twrp_dev/bin/kexec_x86_64_static`，不内置在 ramdisk 中。

---

## 常见问题

### Q: boot.img 大小应该是多少？

当前 `boot_final.img` 约 **15.3MB**，boot 分区已扩展至 **32MB** (原 16MB，通过合并 recovery 分区实现)，剩余约 16.7MB 头部空间。

### Q: 刷入后如何回滚？

```bash
# 回滚到原始 TWRP baseline
fastboot flash boot /home/drie/n1_dev/images/twrp_baseline.img
# 或回滚到完全原厂
fastboot flash boot /home/drie/n1_dev/images/original/stock_boot_cnb19.img
```

### Q: 如何验证打包的镜像？

```bash
file twrp_lk2nd.img
# 应显示: Android bootimg, kernel (0x10008000), ramdisk (0x11000000),
#         second stage (0x10f00000), page size: 2048
```

---

## 参考

- **Boot Image Format**: Android Open Source Project (AOSP)
- **原厂镜像**: `images/original/` 目录
- **硬件参数**: `docs/HARDWARE_REFERENCE.md`
- **开发指南**: `twrp_dev/DEVELOPMENT_GUIDE.md`
- **启动链指南**: `twrp_dev/BOOT_CHAIN_GUIDE.md`
- **GPT 分区分析**: `docs/BOOT_FLOW_PARTITION_ANALYSIS.md`

---

**最后更新**: 2026-03-05  
**方案**: TWRP-based lk2nd (替代已废弃的 Zig Unikernel 方案)
