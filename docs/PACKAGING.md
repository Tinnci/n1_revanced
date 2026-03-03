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

### 1. 修改 ramdisk

```bash
cd /home/drie/n1_dev/twrp_dev/ramdisk_unpacked
# 编辑 init.rc, 添加脚本到 sbin/, 等等
```

### 2. 打包 boot.img

```bash
cd /home/drie/n1_dev/twrp_dev

# 打包 ramdisk
cd ramdisk_unpacked && find . | cpio -o -H newc 2>/dev/null | gzip > ../ramdisk_new.gz && cd ..

# 创建 boot.img
mkbootimg \
    --kernel kernel \
    --ramdisk ramdisk_new.gz \
    --second second \
    --base 0x10000000 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x01000000 \
    --second_offset 0x00f00000 \
    --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx androidboot.serialno=01234567890123456789 gpt bootboost=1" \
    --pagesize 2048 \
    -o twrp_lk2nd.img
```

### 3. 刷入

```bash
adb reboot bootloader
fastboot flash boot twrp_lk2nd.img
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
| ramdisk | `twrp_dev/ramdisk_unpacked/` | 基于 TWRP 2.8，我们修改 | ⚠️ Modified |
| 打包参数 | 本文档 | 从原厂 header 提取 | ✅ Ground Truth |

---

## 我们对 ramdisk 的修改

以下是我们相对于原始 TWRP ramdisk 所做的改动：

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `init.rc` | 修改 | 添加了 RNDIS property 触发器 |
| `sbin/busybox` | 替换 | 原版(动态链接) → 静态编译版(带 devmem)，原版备份为 `busybox.orig` |
| `sbin/kexec` | 新增 | 32-bit 静态 kexec-tools 2.0.28 |
| `sbin/devmem` | 新增 | → busybox 符号链接 |
| `sbin/rndis_setup.sh` | 新增 | RNDIS USB 网络配置脚本 |
| `sbin/usb_init.sh` | 新增 | USB/devmem 初始化脚本 |

---

## 常见问题

### Q: boot.img 大小应该是多少？

当前 `twrp_lk2nd.img` 约 **16MB** — 与原厂 droidboot.img 大小一致。

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
- **硬件参数**: `docs/HARDWARE_SPECS.md`
- **开发指南**: `twrp_dev/DEVELOPMENT_GUIDE.md`
- **lk2nd 实现**: `twrp_dev/LK2ND_IMPLEMENTATION.md`

---

**最后更新**: 2026-03-02  
**方案**: TWRP-based lk2nd (替代已废弃的 Zig Unikernel 方案)
