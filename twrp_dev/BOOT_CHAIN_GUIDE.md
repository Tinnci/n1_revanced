# Nokia N1 启动链与 Kexec 实现指南

**更新时间**: 2026-03-03  
**状态**: ✅ kexec 完全成功！内核跳转后正常运行，所有硬件工作。

---

## 目标架构

```
IFWI (ROM)
  ⬇
Bootstub (Second)
  ⬇
Android Kernel 3.10.62-x86_64_moor (当前 TWRP 的内核)
  ⬇
[我们的目标阶段] TWRP/Ramdisk 环境 (充当 Bootloader)
  ⬇ 1. 通过 kexec 加载新内核 ✅ 内核支持已确认
  ⬇ 2. 或者通过 USB 暴露存储让用户刷入新系统
  ⬇ 3. 提供 Telnet/SSH 救援终端 ✅ RNDIS 工作
  ⬇
Mainline Linux / Ubuntu / PostmarketOS
```

---

## 设备信息 (已验证)

| 项目 | 值 |
|------|-----|
| CPU | Intel Atom Z3580 @ 1.33GHz (4 cores, x86_64) |
| 内核 | 3.10.62-x86_64_moor |
| 用户空间 | 32-bit i386 |
| SELinux | Permissive ✅ |

---

## 当前 boot.img 状态

**文件**: `boot_final.img` (15.2 MB)  
**位置**: `/home/drie/n1_dev/twrp_dev/boot_final.img`  
**状态**: 已刷入设备 boot 分区 (32MB，原 16MB 已通过 GPT 修改扩展)

### 包含的工具 (musl-gcc x86_64 静态链接)

| 组件 | 路径 | 说明 |
|------|------|------|
| strace 6.12 | `/sbin/strace` | 系统调用跟踪 (1.4MB) ✅ |
| devmem2 | `/sbin/devmem2` | 物理内存/MMIO 读写 (34KB) ✅ |
| pmic_tool | `/sbin/pmic_tool` | PMIC 寄存器读写 (38KB) ✅ |
| n1_display | `/sbin/n1_display` | 显示/背光/GPIO 控制 (34KB) ✅ |
| n1_hwinfo.sh | `/sbin/n1_hwinfo.sh` | 硬件信息采集脚本 (5.7KB) ✅ |

> 64-bit kexec 二进制单独存放: `bin/kexec_x86_64_static`，需手动推送到设备 `/tmp/` 使用。

---

## 功能验证状态

### ✅ 已验证工作

| 功能 | 测试结果 |
|------|----------|
| **kexec 命令** | `/sbin/kexec --help` 正常输出 |
| **kexec 系统调用** | 内核支持 (30个符号, sysfs 接口存在) |
| **kexec 跳转** | ✅ 完全成功! 内核启动并运行 (需要 64-bit patched kexec + debugfs mount) |
| **devmem** | 可以读取 `0x00000000 -> 0xC0001454` |
| **RNDIS USB 切换** | `setprop sys.usb.config rndis,adb` 工作 |
| **USB 以 RNDIS 枚举** | 主机看到 18D1:D001 |
| **设备网络接口** | rndis0: 192.168.100.1 |
| **Ping 测试** | 100% 成功 ✅ |
| **tcpsvd 控制台** | 端口 2525 可靠连接 shell ✅ |

### ✅ 之前误判为失败 (已修正)

| 功能 | 实际状态 |
|------|------|
| **kexec 新内核启动** | ✅ **完全成功！** kexec'd 内核正常运行 561 秒直到手动 reboot |
| **SFI 表传递** | ✅ SFI 表在 kexec 后被新内核正确解析 (依赖 hardware_subarch=3) |
| **USB/ADB** | ✅ kexec'd 内核中 USB gadget 和 ADB 正常工作 |

### ⚠️ 已知限制

| 功能 | 问题 |
|------|------|
| **32-bit kexec** | 在 64-bit 内核上加载失败 (memfd_create 不可用，需要 64-bit patched kexec) |
| **Telnet 连接** | 已被 tcpsvd 替代 |

### 🔧 关键技术突破

#### 64-bit Kexec 修复 (2 个 patch)

**Patch 1: memfd_create fallback**
- 问题: kernel 3.10 没有 `memfd_create` 系统调用，64-bit kexec 的 `copybuf_memfd()` 失败并返回 `EFAILED` 而非 `EFALLBACK`，导致不会回退到 `kexec_load` 系统调用。
- 文件: `kexec/kexec.c` line ~1340
- 修复: 将 `EFAILED` 改为 `EFALLBACK`，使其在 memfd_create 失败时回退到传统 `kexec_load` 路径。

**Patch 2: R_X86_64_PC64 重定位**
- 问题: Zig/LLVM `-mcmodel=large` 生成了 `R_X86_64_PC64` (Type 24) 重定位，kexec-tools 不支持，musl 的 `elf.h` 也没定义此常量。
- 文件: `kexec/arch/x86_64/kexec-elf-rel-x86_64.c`
- 修复: 添加 `#ifndef R_X86_64_PC64 #define R_X86_64_PC64 24` 和相应的 case 处理代码。

#### kexec 成功的关键前提条件

1. **必须挂载 debugfs**: `mount -t debugfs none /sys/kernel/debug` — 否则 kexec-tools 无法读取 `boot_params/data`，导致 `hardware_subarch` 为 0 (标准 PC) 而非 3 (INTEL_MID)。
2. **使用 64-bit kexec 二进制**: 32-bit kexec 无法在此内核上工作 (缺少 memfd_create)。
3. **bzImage 加载器**: 此内核的 `xloadflags=0x0001` (仅 XLF_KERNEL_64，无 CAN_BE_LOADED_ABOVE_4G)，因此使用 i386 bzImage 加载器而非 bzImage64 加载器。

#### kexec 误判黑屏的原因

**pstore/ramoops 行为**: pstore 总是显示**上一次** boot 的日志。kexec'd 内核启动后，pstore 驱动读取 RAM 中的旧数据（来自 kexec 之前的 boot），保存到 `/sys/fs/pstore/console-ramoops`，然后清空 RAM 开始记录新日志。我们看到 "Starting new kernel" 以为是最终状态，实际那是旧 boot 的最后一条消息。

**如何区分 kexec boot vs bootloader boot**: kexec boot 的 e820 第一条 entry 从 `0x400` 开始，bootloader boot 从 `0x0` 开始。

### ❓ 待测试

| 功能 | 说明 |
|------|------|
| 主线 Linux 内核 | 需要获取/编译适配的内核 |
| UMS (USB 大容量存储) | 暴露 eMMC 分区 |

---

## RNDIS 使用指南

### 启用 RNDIS 网络

```bash
# 1. 通过 ADB 切换 USB 模式
adb shell setprop sys.usb.config rndis,adb

# 2. 等待几秒让 USB 重新枚举

# 3. 在主机上配置网络 (接口名可能不同)
#    查看接口: ip link show | grep -E "usb|rndis|enp"
sudo ip addr add 192.168.100.2/24 dev enp4s0f3u2u2
sudo ip link set enp4s0f3u2u2 up

# 4. 测试连接
ping 192.168.100.1
```

### 恢复到正常 USB 模式

```bash
adb shell setprop sys.usb.config mtp,adb
```

---

## Kexec 使用指南 (✅ 已验证工作)

### 完整的 kexec 命令序列

```bash
# 0. 推送 64-bit patched kexec 到设备
adb push kexec_x86_64_static /tmp/kexec64
adb shell chmod +x /tmp/kexec64

# 1. 挂载 debugfs (关键！否则 hardware_subarch 不正确)
adb shell mount -t debugfs none /sys/kernel/debug

# 2. 提取当前内核和 ramdisk
adb shell '
dd if=/dev/block/by-name/boot of=/tmp/boot.img bs=4096
kernel_size=$(dd if=/tmp/boot.img bs=1 skip=8 count=4 2>/dev/null | od -A n -t u4 | tr -d " ")
ramdisk_size=$(dd if=/tmp/boot.img bs=1 skip=16 count=4 2>/dev/null | od -A n -t u4 | tr -d " ")
page_size=$(dd if=/tmp/boot.img bs=1 skip=36 count=4 2>/dev/null | od -A n -t u4 | tr -d " ")
kernel_pages=$(( (kernel_size + page_size - 1) / page_size ))
dd if=/tmp/boot.img of=/tmp/current_kernel bs=$page_size skip=1 count=$kernel_pages
ramdisk_start=$(( 1 + kernel_pages ))
ramdisk_pages=$(( (ramdisk_size + page_size - 1) / page_size ))
dd if=/tmp/boot.img of=/tmp/current_ramdisk bs=$page_size skip=$ramdisk_start count=$ramdisk_pages
rm /tmp/boot.img
'

# 3. 加载内核 (使用 -i 跳过 SHA256 完整性检查以加快速度)
adb shell /tmp/kexec64 -l /tmp/current_kernel \
  --initrd=/tmp/current_ramdisk \
  --reuse-cmdline -i

# 4. 验证加载成功
adb shell cat /sys/kernel/kexec_loaded  # 应该输出 1

# 5. (可选) 禁用看门狗并离线 CPU
adb shell '
echo 1 > /sys/devices/virtual/misc/watchdog/disable
for i in 1 2 3; do echo 0 > /sys/devices/system/cpu/cpu$i/online; done
sync
'

# 6. 执行跳转
adb shell /tmp/kexec64 -e

# 7. 等待 ~30s 设备重新出现
adb wait-for-device
```

### 如何验证 kexec 是否成功

kexec boot 和正常 bootloader boot 的区别：

```bash
# kexec boot:
dmesg | grep 'BIOS-e820' | head -1
# → BIOS-e820: [mem 0x0000000000000400-...] (从 0x400 开始)

# 正常 bootloader boot:
dmesg | grep 'BIOS-e820' | head -1
# → BIOS-e820: [mem 0x0000000000000000-...] (从 0x0 开始)
```

### 已验证的 kexec 参数组合

| 参数 | 状态 | 说明 |
|------|------|------|
| `--reuse-cmdline -i` | ✅ 工作 | 推荐：复用当前 cmdline，跳过完整性检查 |
| `--reuse-cmdline --console-vga --reset-vga -i` | ✅ 工作 | 带 VGA 控制台 |
| `--append="..." -i` | ✅ 工作 | 自定义 cmdline |
| (不带 `-i`) | ✅ 工作 | 完整 SHA256 校验，较慢 |

---

## 目录结构

```
/home/drie/n1_dev/
├── twrp_dev/
│   ├── boot_final.img     # 当前刷入的 boot.img ✅ (15.2MB)
│   ├── kernel             # 原始 3.10.62 bzImage (64-bit)
│   ├── second             # Bootstub 1.4 (8KB)
│   ├── ramdisk            # 原始 TWRP ramdisk (gzip)
│   ├── ramdisk_unpacked/  # 解压的 ramdisk (含自定义工具)
│   ├── bin/
│   │   └── kexec_x86_64_static  # 64-bit patched kexec
│   ├── tools_src/         # 自定义工具源码
│   ├── BOOT_CHAIN_GUIDE.md  # 本文件
│   └── DEVELOPMENT_GUIDE.md
├── backups/gpt/           # GPT 备份与修改脚本
├── docs/                  # 硬件参考、分析文档
├── images/original/       # 原厂镜像备份
└── ifwi_analysis/         # IFWI 分析报告
```

---

## 下一步计划

### 优先级 1: 利用 kexec 加载自定义系统
- [ ] 编译 mainline Linux 内核 (适配 Intel Atom Z3580)
- [ ] 构建最小 rootfs (Alpine/Buildroot)
- [ ] 通过 kexec 加载 mainline 内核 + 自定义 ramdisk

### 优先级 2: 自动化 kexec boot chain
- [ ] 在 init.rc 中添加 kexec 自动加载逻辑
- [ ] 实现 boot 菜单 (选择启动不同内核)
- [ ] 将 kexec 配置持久化到 /data 或专用分区

### 优先级 3: PostmarketOS / Ubuntu 移植
- [ ] 从 postmarketOS 获取 Intel Atom 内核
- [ ] 测试显示、触摸、WiFi 等外设
- [ ] 集成完整的用户空间

---

## 编译命令参考

### 重新打包 boot.img

```bash
cd /home/drie/n1_dev/twrp_dev

# 1. 从原始 ramdisk 提取干净副本
cd /tmp && rm -rf clean_ramdisk && mkdir clean_ramdisk && cd clean_ramdisk
zcat /home/drie/n1_dev/twrp_dev/ramdisk | cpio -id

# 2. 添加工具 (只添加需要的文件)
cp /path/to/tools sbin/
chmod 755 sbin/<tools>

# 3. 使用 fakeroot 打包 (关键! 确保 root:root 属主)
fakeroot -- sh -c 'find . | cpio -H newc -o 2>/dev/null | gzip -9 > /tmp/clean_ramdisk.gz'

# 4. 创建 boot.img
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
    --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 gpt bootboost=1" \
    --pagesize 2048 \
    -o boot_final.img

# 5. 刷入 (boot 分区 = 32MB)
adb shell dd if=/dev/zero of=/dev/block/by-name/boot bs=4096 count=8192  # 清零
adb push boot_final.img /tmp/
adb shell dd if=/tmp/boot_final.img of=/dev/block/by-name/boot bs=4096

# 注意: 不要直接从 ramdisk_unpacked/ 打包，其中可能有累积的多余文件导致启动失败。
# 必须从原始 ramdisk 文件提取干净副本后再添加工具。
```

### 编译 64-bit patched kexec-tools (✅ 已验证)

```bash
cd /home/drie/n1_dev/twrp_dev

# 下载源码
curl -LO https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kexec/kexec-tools-2.0.28.tar.xz
tar xf kexec-tools-2.0.28.tar.xz
cd kexec-tools-2.0.28

# Patch 1: 添加缺失的头文件
sed -i '33 a #include <libgen.h>' kexec/arch/i386/x86-linux-setup.c

# Patch 2: 修复 purgatory Makefile
sed -i 's/-Wl,-Map=$(PURGATORY_MAP)//' purgatory/Makefile

# Patch 3: memfd_create fallback (EFAILED → EFALLBACK)
# 在 kexec/kexec.c 的 copybuf_memfd() 函数中:
# 将 memfd_create 失败时的 return EFAILED 改为 return EFALLBACK

# Patch 4: R_X86_64_PC64 重定位支持
# 在 kexec/arch/x86_64/kexec-elf-rel-x86_64.c 中添加:
#   #ifndef R_X86_64_PC64
#   #define R_X86_64_PC64 24
#   #endif
#   case R_X86_64_PC64: { ... }

# 编译 64-bit 静态链接
CC="zig cc -target x86_64-linux-musl -fuse-ld=lld" \
CFLAGS="-static -Os" \
LDFLAGS="-static" \
./configure --host=x86_64-linux-musl --without-xen --without-lzma --without-zlib --sbindir=/sbin

make -j$(nproc)

# 结果: build/sbin/kexec
# ELF 64-bit LSB executable, x86-64, statically linked
# 位置: twrp_dev/bin/kexec_x86_64_static (~200KB)
```

---

## 更新日志

### 2026-03-03

- ✅ **确认 kexec 完全成功！** kexec'd 内核正常启动、运行 561 秒直到手动 reboot
- ✅ 发现 pstore 误导：pstore 显示上一次 boot 的日志，不是当前 (kexec'd) boot
- ✅ 发现 kexec boot 识别标志：e820[0] 从 0x400 开始 (vs bootloader 的 0x0)
- ✅ 修复 64-bit kexec memfd_create fallback (EFAILED → EFALLBACK)
- ✅ 修复 R_X86_64_PC64 重定位支持 (musl 兼容)
- ✅ 确认 debugfs 挂载是 hardware_subarch 正确传递的前提

### 2026-03-02

- ✅ kexec debug 脚本开发
- ✅ kexec 测试载荷开发和测试
- ✅ 32-bit 和 64-bit kexec 对比测试
- ✅ pstore 分析框架建立

### 2026-01-18

- ✅ 验证 kexec 系统调用支持 (30 符号, sysfs 接口)
- ✅ 编译 32-bit 静态 kexec-tools 2.0.28 (174KB)
- ✅ 编译 64-bit 静态 kexec-tools (patched)
- ✅ 验证 devmem 工作正常
- ✅ 打包新的 twrp_lk2nd.img 包含 kexec
- ✅ 添加 RNDIS property 触发器到 init.rc
- ✅ RNDIS USB 网络配置成功 (Ping 100% 成功)
- ✅ 实现基于 `tcpsvd` 的可靠 shell 访问 (端口 2525)
