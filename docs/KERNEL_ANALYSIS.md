# Nokia N1 内核深度分析报告

## 1. 启动流程验证

### 文档一致性检查

对比 `BOOT_FLOW_PARTITION_ANALYSIS.md` 和 `BOOTLOADER_DISASSEMBLY_ANALYSIS.md` 中描述的启动链与实际 dmesg 观测：

| 启动阶段 | 文档描述 | 实际观测 | 一致性 |
|----------|---------|---------|--------|
| Boot ROM → IFWI | 从 SPI NOR 加载 | 无直接观测（pre-kernel） | N/A |
| SCU Firmware | VxWorks on Lakemont | `intel_mid_rpmsg` 探测成功，OSHOB 识别 TANGIER\|ANNIEDALE | ✅ |
| IAFW 决策 | 按 GPT 名称查找 "boot" | e820 首条 0x0（非 kexec），分区表正确 | ✅ |
| Bootstub 1.4 | 在 0x10f00000 执行 | ESP 设置与文档描述一致 | ✅ |
| Bootstub → 内核 | 设置 boot_params, 跳转 0x10008000 | `X86_SUBARCH_INTEL_MID` 正确打印 | ✅ |
| SFI 表解析 | IAFW 构建 SFI 表 | 12 个 SFI 表: APIC, CPUS, DEVS, FREQ, GPIO, MCFG, MMAP, OEM0, OEMB, SYST, WAKE, XSDT | ✅ |
| 内核启动 | 3.10.62-x86_64_moor, 64位 | `Linux localhost 3.10.62-x86_64_moor #1 SMP PREEMPT` x86_64 | ✅ |
| Init 进程 | `/init` (ramdisk) | `/proc/1/exe -> /init` | ✅ |
| tngdisp 加载 | `insmod /lib/modules/tngdisp.ko` | `init.recovery.mofd_v1.rc: insmod /lib/modules/tngdisp.ko` at early-init | ✅ |
| DRM 初始化 | drm + pvrsrvkm | `Initialized drm 1.1.0`, `Initialized pvrsrvkm 8.1.0` | ✅ |

### E820 内存映射

```
BIOS-e820: [0x00000000 - 0x00097fff] usable        (608 KB, 低端内存)
           [0x000a0000 - 0x000fffff] reserved       (384 KB, 传统VGA/BIOS)
           [0x00100000 - 0x03ffffff] usable         (63 MB)
           [0x04000000 - 0x051fffff] reserved       (18 MB, 可能是固件/GPU保留)
           [0x05200000 - 0x05bfffff] usable         (10 MB)
           [0x05c00000 - 0x05ffffff] reserved       (4 MB)
           [0x06000000 - 0x3fffffff] usable         (928 MB)
           [0x40000000 - 0x40000fff] reserved       (4 KB, MMIO hole)
           [0x40001000 - 0x7edfffff] usable         (1006 MB)
           [0x7ee00000 - 0x7fffffff] reserved       (18 MB, 顶端固件保留)
           [0xfec00000 - 0xfec00fff] reserved       (IOAPIC)
           [0xfec04000 - 0xfec07fff] reserved       (HPET? 其他MMIO)
           [0xfee00000 - 0xfee00fff] reserved       (Local APIC)
           [0xff000000 - 0xffffffff] reserved       (16 MB, SPI NOR/IFWI)
```

**总可用 RAM**: ~2007 MB（与 `Memory: 1981276k/2078720k available` 吻合）

### 与文档的关键差异

1. **文档误差：物理内存不在 0x10000000 开始**

   `BOOTLOADER_DISASSEMBLY_ANALYSIS.md` 推断 "所有 RAM 在 0x10000000+"，但实际 e820 显示 RAM 从 0x0 开始。0x10000000 是 `boot.img` 头部的 **base_addr** 参数，不是物理 RAM 起始地址。Bootstub 使用此值计算加载地址偏移，并非物理映射的起点。

   **结论**：Boot.img 的 base=0x10000000 是一个**虚拟约定**，Bootstub 根据它计算各组件的相对偏移。实际内核被加载到物理内存的低端区域，而非 0x10008000 物理地址。

2. **Bootstub cmdline 替换确认**

   文档未明确描述 Bootstub 替换 cmdline 中占位符的行为。实际验证：
   - `xxxx:xxxx:xxxx:xxxx:xxxx:xxxx` → `0000:0000:0000:0008:0000:0000`
   - `01234567890123456789` → `L25110TYN1K3`
   - Bootstub 还添加了: `androidboot.wakesrc=05`, `androidboot.mode=main`, `androidboot.bootreason=kernel_panic`

3. **启动顺序中 `remoteproc` 的角色**

   文档未提及 `remoteproc`。实际观测：内核启动早期（0.186s）就加载了 `intel_mid_remoteproc.fw`（4.4KB，32-bit x86 ELF），这是 SCU/IA 核心间通信的固件，负责管理 RPMSG（Remote Processor Messaging）通道。

---

## 2. 内核构建信息

```
版本:      3.10.62-x86_64_moor
编译器:    GCC 4.8 (GNU)
构建者:    sam@topaz4
构建时间:  Wed Nov 25 10:05:47 CST 2015
配置:      SMP PREEMPT
架构:      x86_64 (Intel MID subarch)
CONFIG_IKCONFIG: 未启用 (无 /proc/config.gz)
```

### CPU 信息

```
型号:      Intel Atom Z3580 @ 1.33GHz (Silvermont 微架构)
核心:      4 核
微码:      0x38
实际频率:  动态可达 2333 MHz
缓存:      1024 KB L2
```

**CPU 特性标志**: fpu, vme, de, pse, tsc, msr, pae, mce, cx8, apic, sep, mtrr, pge, mca, cmov, pat, pse36, clflush, dts, acpi, mmx, fxsr, **sse**, **sse2**, ss, ht, tm, pbe, syscall, nx, rdtscp, lm, constant_tsc, arch_perfmon, pebs, bts, rep_good, nopl, xtopology, nonstop_tsc, aperfmperf, pni, **pclmulqdq**, dtes64, monitor, ds_cpl, **vmx**, est, tm2, **ssse3**, cx16, xtpr, pdcm, **sse4_1**, **sse4_2**, movbe, popcnt, tsc_deadline_timer, **aes**, rdrand, lahf_lm, 3dnowprefetch, ida, arat, epb, dtherm, tpr_shadow, vnmi, flexpriority, ept, vpid, tsc_adjust, smep, erms

**关键能力**: SSE4.2, AES-NI, VT-x(VMX), RDRAND, PCLMULQDQ

### 内核二进制结构

```
bzImage (kernel):
  总大小:         6,330,768 bytes (6.0 MB)
  Setup code:     15,360 bytes (30 sectors)
  Gzip payload:   @0x3E4D, 6,294,414 bytes
  
vmlinux (解压后):
  格式:           ELF64 x86-64
  总大小:         22,292,672 bytes (21.3 MB)
  .text:          8,715,749 bytes (8.3 MB, AX)
  .rodata:        3,511,182 bytes (3.3 MB, A)
  .data:          2,141,888 bytes (2.0 MB, WA)
  .bss:           8,654,848 bytes (8.3 MB, WA)
  .init.text:     336,289 bytes (328 KB, AX)
  .init.data:     760,712 bytes (743 KB, WA)
  
符号表:           已剥离（无 .symtab）
  /proc/kallsyms: 85,274 个符号（运行时导出）
```

---

## 3. 内核模块清单

### 外部可加载模块

| 模块 | 大小 | 位置 | 描述 | 许可证 |
|------|------|------|------|--------|
| **tngdisp.ko** | 2,411,364 B (2.3 MB) | /lib/modules/ | Intel Tangier 显示/GPU 驱动 | Dual MIT/GPL |

`tngdisp.ko` 是一个**超级模块**，实际集成了：
- **DRM/KMS 框架** (psb_drm, Intel GMA500 兼容层)
- **PowerVR SGX544MP2 GPU 驱动** (pvrsrvkm 8.1.0, Imagination Technologies)
- **MIPI DSI 显示控制器**
- **HDMI 输出**
- **视频解码加速** (MSVDX - Intel 视频解码引擎)
- **视频编码加速** (TOPAZ HP - Intel 视频编码引擎)
- **视频信号处理** (VSP - Video Signal Processor)
- **VEC** (Video Encode Core)

**加载时机**: `init.recovery.mofd_v1.rc` 的 `on early-init` 阶段

### 内置模块（104 个）

按功能分类的关键内置驱动：

**平台核心 (Intel MID)**:
- `intel_mid` - MID 平台核心
- `intel_soc_pmu` - 电源管理单元
- `intel_scu_watchdog_evo` - SCU 看门狗
- `intel_mid_dma` - DMA 控制器
- `intel_mid_osip` - OSIP 启动管理
- `intel_fabric_logging` - Fabric 错误日志
- `sfi_cpufreq` - SFI 频率调节
- `intel_idle` - CPU 空闲状态

**安全引擎**:
- `sep54` - Discretix SEP54 安全协处理器（加密/DRM/安全启动）

**传感器**:
- `psh` - Intel 传感器集线器（加速度计/陀螺仪/磁力计）

**USB**:
- `dwc3_intel_mrfl` - DWC3 USB3 控制器
- `xhci_hcd` - xHCI 主机控制器
- `ehci_hcd` - EHCI USB2 主机控制器
- `g_android` - USB Gadget (ADB/MTP)
- `otg` - USB OTG 切换

**存储**:
- `mmc_core`, `mmcblk` - eMMC 存储
- `sdhci` - SD 主机控制器

**网络/通信**:
- `rfkill` - 无线开关
- `cdc_ncm` - USB 网络
- `asix` - USB 以太网

**音频**:
- `snd` 系列 - 音频子系统

**输入**:
- `hid`, `usbhid` - HID 输入
- `xpad` - Xbox 手柄兼容

**调试**:
- `pstore`, `ramoops` - 崩溃日志持久化
- `oprofile` - 性能分析
- `logger_pti` - PTI 日志

---

## 4. 闭源/二进制驱动分析

### 4.1 tngdisp.ko — 显示/GPU 超级模块

**类型**: 可加载内核模块 (.ko)
**大小**: 代码 1,091,393 B + 数据 25,480 B + BSS 126,192 B = 总计 ~1.2 MB
**符号数**: 5,463 个（4,795 个函数 + 数据符号）
**许可证声明**: Dual MIT/GPL

**子系统构成**:

| 子系统 | 前缀 | 描述 | 闭源程度 |
|--------|------|------|---------|
| pvrsrvkm | `PVRSRV*`, `PVR*` | PowerVR GPU 内核服务 | 🔴 完全闭源 |
| psb_drm | `psb_*` | Intel GMA500 DRM 框架 | 🟡 半开源 |
| MSVDX | `psb_msvdx_*` | 视频解码引擎 | 🔴 闭源+固件 |
| TOPAZ HP | `tng_topaz_*` | 视频编码引擎 | 🔴 闭源+固件 |
| VSP | `vsp_*` | 视频信号处理 | 🔴 闭源+固件 |
| MIPI DSI | `mdfld_dsi_*` | 显示控制器 | 🟡 半开源 |
| HDMI | `hdmi_*` | HDMI 输出 | 🟡 半开源 |

**需要的固件文件**:
- `topazhp_fw.bin` - 视频编码器固件（运行时从 /lib/firmware 加载）
- `msvdx_fw_mfld_DE2.0.bin` - 视频解码器固件

**模块参数** (49 个):

```
显示:       PanelID, force_pipeb, dsr, smart_vsync, te_delay, vblank_sync
            no_fb, default_hdmi_scaling_mode, hdmi_state, hdmi_edid, hdmi_hpd_auto
GPU:        gPVRDebugLevel, debug, trap_pagefaults, gfx_pm, rtpm, ospm
视频解码:   msvdx_pmpolicy, msvdx_bottom_half, msvdx_tiling_memory, ved_pm, decode_flag
视频编码:   topaz_pmpolicy, topaz_cmdpolicy, topaz_cgpolicy, topaz_clockgating
            topaz_pmlatency, topaz_sbuswa, vec_pm, vec_force_down_freq, vec_force_up_freq
VSP:        vsp_pm, vsp_pmpolicy, vsp_force_up_freq, vsp_force_down_freq
            vsp_burst, vsp_single_int, vsp_vpp_batch_cmd, video_sepkey
色彩:       enable_color_conversion, enable_gamma, gamma_adjust, csc_adjust
            CABC_control, LABC_control, psb_enable_pr2_cabc
其他:       cpu_relax, udelay_divider, maxfifo_delay
```

### 4.2 sep54 — Discretix 安全引擎

**类型**: 内核内置（built-in）
**代码大小**: ~47.9 KB（43 个导出函数）
**描述**: Discretix（现 ARM TrustZone CryptoCell）SEP54 安全协处理器驱动

**功能**:
- 硬件加密加速（AES, RSA, SHA, HMAC）
- 安全密钥存储和管理
- DRM 内容保护
- RPMB 安全存储代理
- 安全启动验证

**固件**: ROM内置 (v1.2.5.1) + FW (v1.3.1)，**不需要**外部文件加载

**模块参数**: `dcache_size_log2`, `icache_size_log2`, `sep_log_level`, `sep_log_mask`, `disable_linux_crypto`, `q_num`

**闭源状态**: ✅ **源码已找到** — CyanogenMod 内核中 `drivers/staging/sep54/` 包含 10+ 个源文件（sep_driver.c, sep_applets.c, crypto_api.c 等）。设备上运行的是编译后的二进制，但可通过源码重新编译。

### 4.3 psh — Intel 传感器集线器

**类型**: 内核内置
**代码大小**: ~34.0 KB（26 个导出函数）
**描述**: Intel Platform Sensor Hub (ISH) 驱动

**功能**:
- 管理加速度计/陀螺仪/磁力计等传感器
- 传感器数据融合
- 低功耗传感器轮询

**固件**: `psh.bin`（通过 IPC 从主处理器加载到传感器集线器处理器）

**模块参数**: `disable_psh_recovery`, `force_psh_recovery`

**闭源状态**: 🟠 驱动开源（Intel 公开了 ISH 驱动源码），但固件二进制闭源。

### 4.4 dwc3_intel_mrfl — USB3 控制器

**类型**: 内核内置
**代码大小**: ~53.5 KB（15 个导出函数）
**描述**: Synopsys DWC3 USB3 控制器的 Intel Moorefield 定制版

**闭源状态**: ✅ **源码已找到** — CyanogenMod 内核中 `drivers/usb/dwc3/dwc3-intel-mrfl.c` 包含完整的 Intel Moorefield 平台特定代码。

### 4.5 intel_mid_remoteproc — SCU 通信固件

**类型**: 内核内置固件 (.builtin_fw)
**大小**: 4,456 bytes
**格式**: ELF32 x86 可执行文件
**入口点**: 0x24

**描述**: IA 核心与 SCU (Lakemont) 核心之间的 RPMSG 通信固件。在内核启动 0.186 秒时加载。

**闭源状态**: 🔴 完全闭源二进制。

---

## 5. 内核编译与调整可行性

### 5.1 内核源码可用性

| 来源 | 状态 | 说明 |
|------|------|------|
| Nokia 官方 | ❌ 未公开 | Nokia N1 从未发布内核源码 |
| **CyanogenMod/android_kernel_asus_moorefield** | ✅ **最佳选择** | [GitHub](https://github.com/CyanogenMod/android_kernel_asus_moorefield) cm-14.1 分支, 内核 3.10.72 |
| LineageOS/android_kernel_asus_moorefield | ✅ 可用 | CyanogenMod 的延续，lineage-15.0 分支 |
| RaphielGang/kernel_asus_moorefield | ✅ 原厂dump | ASUS 原始源码转储 V4.21.40 (597MB) |
| Android kernel 3.10 (AOSP) | 🟡 基础 | 缺少 Intel MID BSP 补丁 |
| PowerVR GPU (tngdisp) | ✅ **有源码** | 完整 PVR RGX + DRM + MSVDX/TOPAZ 源码在 `drivers/external_drivers/intel_media/` |
| sep54 | ✅ **有源码** | 在 `drivers/staging/sep54/` 下 |
| psh (传感器) | ✅ **有源码** | 在 `drivers/external_drivers/drivers/hwmon/psh.c` |

**已验证**: CyanogenMod 内核源码已克隆到 `/home/drie/n1_dev/kernel_source/`

#### 兼容性验证结果

| 检查项 | ZenFone 2 源码 | Nokia N1 设备 | 匹配度 |
|--------|---------------|-------------|--------|
| SoC | Intel Z3580 (Moorefield) | Intel Z3580 (Moorefield) | ✅ 完全匹配 |
| 内核版本 | 3.10.72 | 3.10.62 | 🟡 +10 sublevel (可降级) |
| 架构 | x86_64, Intel MID, SFI | x86_64, Intel MID, SFI | ✅ 完全匹配 |
| defconfig | `x86_64_moor_defconfig` (4142 行) | — | ✅ 可用 |
| 显示面板 | **JDI_7x12_VID** (PanelID=11) | **JDI_7x12_VID** (PanelID=11) | ✅ **完全匹配** |
| tngdisp 驱动 | `jdi_vid.c` + DRM/PVR 全套源码 | tngdisp.ko | ✅ 源码可用 |
| sep54 | `drivers/staging/sep54/` (10个源文件) | 内置 | ✅ 源码可用 |
| DWC3 USB | `dwc3-intel-mrfl.c` | 内置 | ✅ 源码可用 |
| 模块签名 | `CONFIG_MODULE_SIG_FORCE=y` (SHA256) | `Magrathea: Glacier signing key` | ⚠️ 需要新密钥 |

### 5.2 编译工具链

```
必需工具链:  GCC 4.8 (x86_64 交叉编译)
推荐方式:    Android NDK r10e 或 Linaro GCC 4.8
内核版本:    3.10.62 (android-3.10 分支)
```

### 5.3 可行的内核调整

#### ✅ 不需要重编译即可调整

1. **tngdisp.ko 模块参数**（修改 init.rc 或 modprobe.conf）：
   ```bash
   insmod /lib/modules/tngdisp.ko gPVRDebugLevel=0 ospm=1 smart_vsync=1
   ```

2. **内核 cmdline 参数**（修改 boot.img）：
   - `loglevel=7` — 增加启动日志详细度
   - `printk.time=1` — 带时间戳的 printk
   - `androidboot.selinux=permissive` — 禁用 SELinux 强制模式
   - `intel_idle.max_cstate=N` — 限制 CPU 空闲深度
   - `nmi_watchdog=0` — 禁用 NMI 看门狗

#### 🟡 需要编译新内核

0. **启用 CONFIG_IKCONFIG** — 让 /proc/config.gz 可用于后续分析
1. **启用 KDB/KGDB** — 内核调试器
2. **添加 ftrace/perf 支持** — 性能分析
3. **更新 USB gadget 配置** — 支持更多 USB 模式
4. **启用 CIFS/NFS** — 网络文件系统
5. **启用 iptables 完整功能** — 防火墙/NAT
6. **调整 PREEMPT 模型** — 影响延迟/吞吐量
7. **移除不需要的驱动** — 减小内核大小

#### ❌ 不可行或有限制

1. **升级到高版本内核 (4.x/5.x)** — Intel MID BSP 全部基于 3.10，GPU/显示/安全驱动无法迁移
2. **使用 Mesa 替代 PowerVR GPU** — Mesa 的 pvr 支持是 2024+ 的新项目，不适配 3.10 内核
3. **完全脱离 Intel 闭源固件** — remoteproc.fw、topazhp_fw.bin、msvdx_fw.bin 仍为二进制 blob

> **注意**: 此前标记为"闭源"的 tngdisp、sep54、psh、dwc3 驱动的**源码**已在 CyanogenMod 内核仓库中找到，可以重新编译和修改。真正不可替代的闭源部分仅限于**固件二进制文件**。

### 5.4 编译新内核的步骤

```bash
# 1. 源码已就绪
cd /home/drie/n1_dev/kernel_source  # CyanogenMod cm-14.1, 3.10.72

# 2. 使用匹配的 defconfig
export ARCH=x86_64
export CROSS_COMPILE=x86_64-linux-android-
make x86_64_moor_defconfig

# 3. 关键修改：启用 IKCONFIG, 禁用模块强制签名
scripts/config --enable CONFIG_IKCONFIG
scripts/config --enable CONFIG_IKCONFIG_PROC
scripts/config --disable CONFIG_MODULE_SIG_FORCE
# 或者生成自己的签名密钥

# 4. 编译
make -j$(nproc) bzImage
make -j$(nproc) modules    # 编译 tngdisp.ko

# 5. 打包 boot.img
mkbootimg --kernel arch/x86/boot/bzImage \
  --ramdisk /tmp/clean_ramdisk.gz \
  --second /home/drie/n1_dev/twrp_dev/second \
  --base 0x10000000 --kernel_offset 0x00008000 \
  --ramdisk_offset 0x01000000 --second_offset 0x00f00000 \
  --tags_offset 0x00000100 --pagesize 2048 \
  --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M \
androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 \
androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx \
androidboot.serialno=01234567890123456789 gpt bootboost=1 panic=15" \
  --output boot_custom.img
```

**关键注意事项**:

1. **GCC 版本**: 必须使用 GCC 4.8（原始编译器），推荐 Android NDK r10e 中的 x86_64 交叉工具链
2. **vermagic 匹配**: 如果要加载设备上原有的 tngdisp.ko，编译出的内核 vermagic 必须为 `3.10.62-x86_64_moor SMP preempt mod_unload`——需要将 Makefile 的 SUBLEVEL 从 72 改为 62
3. **模块签名**: 原始 tngdisp.ko 使用 `Magrathea: Glacier signing key` 签名。如需加载自编译模块，要么禁用 MODULE_SIG_FORCE，要么用自己的密钥重新签名
4. **面板驱动**: Nokia N1 使用 JDI_7x12_VID (PanelID=11)，ZenFone 2 源码中已包含此面板的完整驱动 `jdi_vid.c`
5. **remoteproc 固件**: intel_mid_remoteproc.fw (4.4KB) 已内嵌在内核 .builtin_fw 段中，源码编译时需要此固件文件

---

## 6. 已提取的闭源二进制文件

所有文件保存在 `/tmp/n1_drivers/`:

| 文件 | 大小 | 来源 | 描述 |
|------|------|------|------|
| `tngdisp.ko` | 2,411,364 B | 设备 /lib/modules/ | 显示/GPU 超级模块 (ELF64 x86-64 .ko) |
| `intel_mid_remoteproc.fw` | 4,456 B | 内核 .builtin_fw 段 | SCU-IA 通信固件 (ELF32 x86) |
| `sep54_crypto.bin` | 30,080 B | vmlinux .text @0x82601a80 | 安全引擎驱动代码 (raw x86-64) |
| `psh_sensor_hub.bin` | 34,768 B | vmlinux .text @0x8264d830 | 传感器集线器驱动代码 (raw x86-64) |
| `dwc3_intel_usb.bin` | 54,816 B | vmlinux .text @0x824c49e0 | USB3 控制器驱动代码 (raw x86-64) |
| `kallsyms.txt` | 3,572,047 B | /proc/kallsyms | 完整运行时符号表 (85,274 symbols) |
| `vmlinux_raw` | 22,292,672 B | 解压自 bzImage | 完整内核 ELF (in /tmp/) |

### 符号完整性

尽管 vmlinux ELF 中**没有** .symtab 段（已被 strip），运行时 /proc/kallsyms 提供了完整的 85,274 个符号映射，包括：
- 38,855 个函数符号 (vmlinux 内)
- 4,795 个函数符号 (tngdisp.ko)
- 数据/BSS/只读数据符号

这意味着我们可以利用 kallsyms 对任何内核函数进行**完整的反汇编和逆向分析**。

---

## 7. 结论与建议

### 启动流程验证结论

文档中描述的启动流程与实际观测**高度一致**。唯一需要修正的是关于物理内存映射的描述——boot.img 中的 base_addr=0x10000000 不代表物理 RAM 起始地址，而是一个 Bootstub 使用的约定偏移量。

### 内核调整建议

1. **短期（无需编译）**: 优化 cmdline 参数和 tngdisp.ko 模块参数
2. **中期（源码已就绪）**: 使用 CyanogenMod 源码编译自定义内核，启用 IKCONFIG、禁用 MODULE_SIG_FORCE、添加调试功能
3. **长期**: 适配 Nokia N1 特定硬件差异（如电池管理、触摸屏控制器），构建完整的可量产内核

### 闭源驱动风险评估

| 驱动 | 可替代性 | 影响 |
|------|---------|------|
| tngdisp.ko (GPU/显示) | ❌ 不可替代 | 无此模块=无屏幕输出 |
| sep54 (安全引擎) | ✅ 可禁用 | 失去硬件加密加速，DRM 受影响 |
| psh (传感器) | ✅ 可禁用 | 失去传感器功能（加速度计等） |
| remoteproc | ❌ 不可替代 | IPC 通信基础设施，影响多个子系统 |
| dwc3_intel | 🟡 可替换为通用版 | USB 功能可能受限 |
