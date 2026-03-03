# Nokia N1 TWRP 构建指南

## 概述

本目录包含 Nokia N1 (Intel Moorefield/mofd_v1) 的 TWRP Recovery 设备树。

### 设备参数

| 属性 | 值 |
|------|-----|
| 设备 | Nokia N1 |
| 代号 | Nokia_N1 / mofd_v1 |
| SoC | Intel Atom Z3580 (Moorefield) |
| 架构 | x86_64 内核 + x86 (32-bit) 用户空间 |
| 内核 | 3.10.62-x86_64_moor (原厂固件) |
| 屏幕 | 2048×1536 (TWRP UI 渲染为 1080×1920) |
| eMMC | PCI 路径 `/dev/block/pci/pci0000:00/0000:00:01.0/by-name/` |
| 加密 | FDE, footer 在 `/factory/userdata_footer` |
| Android 版本 | 5.1.1 (Lollipop) |

## 文件结构

```
Nokia_N1/
├── AndroidProducts.mk          # AOSP 产品定义
├── BoardConfig.mk              # 核心硬件配置 (分区/内核/TWRP flags)
├── device.mk                   # 产品 makefile
├── omni_Nokia_N1.mk            # lunch target 定义
├── recovery.fstab              # Android 标准 fstab
├── init.recovery.mofd_v1.rc    # 硬件初始化 (显示/USB/看门狗)
├── ueventd.mofd_v1.rc          # 设备节点权限
├── build_twrp.sh               # 一键构建脚本
├── BUILD_GUIDE.md              # 本文件
├── prebuilt/
│   ├── kernel                  # 原厂 bzImage (6.3MB, 64-bit)
│   └── second                  # Intel Bootstub 1.4 (8KB)
└── recovery/root/
    ├── etc/
    │   └── twrp.fstab          # TWRP 分区布局 (PCI 路径)
    ├── intel_prop.cfg           # 固件版本属性配置
    ├── lib/modules/
    │   └── tngdisp.ko          # Tangier 显示驱动 (64-bit模块, 2.3MB)
    └── sbin/
        └── lauchrecovery       # recovery 启动脚本 (先加载tngdisp.ko)
```

## 构建方法

### 方法一：完整 AOSP/TWRP 构建 (推荐用于修改 TWRP 源码)

需要 ~50GB 磁盘空间和 Android 构建环境。

```bash
# 1. 安装依赖
./build_twrp.sh deps

# 2. 同步 TWRP 源码树 (数GB下载)
./build_twrp.sh sync

# 3. 构建 recovery image
./build_twrp.sh build

# 或一步完成
./build_twrp.sh all
```

输出: `~/twrp_build/out/target/product/Nokia_N1/recovery.img`

### 方法二：手动打包 (仅修改 ramdisk，无需 AOSP 构建系统)

只需 `mkbootimg` 工具。这是**最快的方法**，适用于只修改 ramdisk 内容的场景。

```bash
# 安装 mkbootimg
pip3 install mkbootimg
# 或从 Android SDK platform-tools 获取

# 使用脚本打包
./build_twrp.sh pack
```

手动打包命令：

```bash
# 1. 打包 ramdisk
cd /path/to/ramdisk_unpacked
find . | cpio -o -H newc | gzip -9 > /tmp/ramdisk.cpio.gz

# 2. 用 mkbootimg 生成 boot.img
mkbootimg \
    --kernel prebuilt/kernel \
    --ramdisk /tmp/ramdisk.cpio.gz \
    --second prebuilt/second \
    --base 0x10000000 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x01000000 \
    --second_offset 0x00f00000 \
    --tags_offset 0x00000100 \
    --pagesize 2048 \
    --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 gpt bootboost=1" \
    -o twrp_nokia_n1.img
```

## 刷入方法

### 方法 A: Fastboot
```bash
fastboot flash boot twrp_nokia_n1.img
fastboot flash recovery twrp_nokia_n1.img
```

### 方法 B: 在 recovery 中 dd
```bash
adb push twrp_nokia_n1.img /tmp/
adb shell dd if=/tmp/twrp_nokia_n1.img of=/dev/block/by-name/boot
adb shell dd if=/tmp/twrp_nokia_n1.img of=/dev/block/by-name/recovery
```

### 方法 C: Kexec (不写入闪存)
```bash
adb push twrp_nokia_n1.img /tmp/
adb shell /sbin/kexec -l /tmp/twrp_nokia_n1.img --type=bzImage
adb shell /sbin/kexec -e
```

## 关键技术细节

### 为什么需要 second bootloader？

Nokia N1 使用 Intel SFI (Simple Firmware Interface) 引导流程：

```
IFWI → Droidboot → Bootstub 1.4 (second) → Linux Kernel → init
```

`second` 包含 Intel Bootstub 1.4，它负责：
1. 从 Droidboot 的 32-bit 保护模式过渡到 Linux 启动
2. 设置 Linux boot_params 结构
3. 处理 SFI 硬件表

**如果不包含 second，启动会失败**。

### 32-bit 用户空间 + 64-bit 内核

- 内核: `x86_64` (64-bit)
- TWRP: `i386` (32-bit)
- 原因: TWRP 2.8 时代默认编译 32-bit x86 userspace
- 内核的 `CONFIG_IA32_EMULATION=y` 使其工作
- `TARGET_ARCH := x86` (不是 x86_64)

### 显示驱动 (tngdisp.ko)

- 64-bit 内核模块 (ELF 64-bit x86_64)
- 必须在 recovery 启动前加载 (`insmod`)
- 由 `lauchrecovery` 脚本处理
- 驱动 PowerVR GPU 的 DSI 显示输出

### 分区布局

Nokia N1 使用 GPT 分区 + Intel PCI eMMC 控制器。
块设备路径格式: `/dev/block/pci/pci0000:00/0000:00:01.0/by-name/<name>`

在 init.recovery.mofd_v1.rc 中创建简写符号链接:
```
/dev/block/pci/pci0000:00/0000:00:01.0/by-name → /dev/block/by-name
```

### 加密支持

- 类型: Android FDE (Full Disk Encryption)
- 加密元数据: `/factory/userdata_footer` (在 factory 分区上)
- TWRP 需要: `TW_INCLUDE_CRYPTO := true`
- factory 分区在 init 阶段挂载以读取加密 footer

## 已知问题

1. **TWRP UI 分辨率不匹配**: 面板是 2048×1536 (4:3)，但 TWRP UI 渲染为 1080×1920 (16:9)
2. **kexec 后屏幕黑屏**: 如果通过 kexec 重启，旧内核会关闭显示面板 (GPIO-189/190)，新内核不会重新初始化

## 分区布局详情 (已验证, 2025-03-03 更新)

> **注意**: boot 分区已从 16MB 扩展至 32MB，recovery 分区已删除（合并入 boot）。

| 分区 | 大小 | 说明 |
|------|------|------|
| **boot** | **32 MB** | boot.img (kernel + ramdisk + bootstub + 调试工具) |
| fastboot | 16 MB | Droidboot |
| splashscreen | 4 MB | 开机画面 |
| system | 2 GB | Android 系统 |
| cache | 1.5 GB | 缓存/OTA |
| data | 24.8 GB | 用户数据 (加密) |
| misc | 128 MB | bootloader 通信 |
| persistent | 1 MB | FRP (Factory Reset Protection) |
| config | 128 MB | 配置数据 |
| logs | 128 MB | 日志 |
| factory | 128 MB | 工厂数据 + 加密 footer |
