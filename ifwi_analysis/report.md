# Nokia N1 IFWI (ifwi_boot0.bin) 深度分析报告

**版本**: E123.2001 (用户提供)  
**文件大小**: 4,194,304 字节 (4MB)  
**分析日期**: 2026-01-18  

---

## 1. IFWI 概述

IFWI (Integrated Firmware Image) 是 Intel SoC 的综合固件镜像，包含了从 CPU 复位到操作系统加载前的所有固件组件。对于 Nokia N1 的 Intel Moorefield (Z3580) 平台，IFWI 是最底层的固件。

### 文件格式标识

| 偏移 | 魔数 | 描述 |
|------|------|------|
| 0x0000 | `UMIP` | Unified MIP (Manufacturer Info Page) |
| 0x0040 | `iMRC` | Intel Memory Reference Code 标记 |
| 0x2048 | `MMRC` | Memory MRC 配置数据 |
| 0x10080 | `MIPNS` | MIP Non-Secure 分区 (签名) |
| 0x103e8 | `IMRN` | Isolated Memory Region 配置 |
| 0x10538 | `CARG` | CPU 参数配置 |
| 0x10ad8 | `SSAC` | Secure Storage Access Control |
| 0x11480 | `MIPDS` | MIP Data Secure (签名) |
| 0x3a080 | `IAFWS` | IA Firmware Secure (x86 固件，签名) |

---

## 2. 平台信息

### SoC 识别
- **平台代号**: ANN (Anniedale) / TNG (Tangier)
- **完整名称**: Intel Moorefield Z3580
- **步进版本**: 支持 B0, B1 stepping (从 RTL Stepping 字符串可见)
- **芯片组**: Intel Tangier 系列

### 固件组件版本
从字符串分析提取:
```
Chaabi ANN UMCHID 1.1b     - Chaabi 安全处理器标识
Chaabi ANN ProvBaseGC 1.1b - 基础安全配置
Chaabi ANN ProvGC 1.1b     - 安全配置
Chaabi ANB RandGC 1.1b     - 随机数生成器
Chaabi ANN HashGC 1.1b     - 哈希计算模块
```

### 内存配置标识
```
BVCX...000KDCNI  - 内存配置标识符
```
`000KDCNI` 可能代表内存类型/配置 (LPDDR3 配置代码)

---

## 3. 固件分区结构

### 主要分区表 (从字符串解析)

```
SPCT  - SPI Controller Table
SPAT  - SPI Address Table  
RPCH  - ROM Patch
MIPN  - MIP Non-secure
MIPD  - MIP Data
CH00-CH17 - Chaabi 分区 (18个安全子模块)
SCuC  - SCU 控制器代码 (System Controller Unit)
$mIA  - IA (Intel Architecture) 代码标记
IAFW  - IA Firmware (x86 启动代码)
$VED  - Video Encoder/Decoder
$VEC  - Video Codec
$VSP  - Video Signal Processor
MOFW  - Modem Firmware 标记
$MOS  - Modem OS
$POS  - Platform OS
$ROS  - Recovery OS
$SLL  - Secure LL
$TOS  - Trusted OS
$DOS  - Download OS
$DnX  - DnX 模式标记
DFRM  - DFU/Recovery Mode
DORM  - Dormant Mode
DxxM  - DnX Mode
$SSI  - Secure Storage Interface
$PSH  - Platform Sensor Hub
$PSHK - PSH Kernel
STR   - String Table
$BLDR - Bootloader
DOS   - Download OS
$BOS  - Boot OS
```

---

## 4. 核心组件分析

### A. SCU (System Controller Unit)
Intel Moorefield 的低功耗微控制器，负责:
- 电源管理 (PMIC 控制)
- 看门狗
- 中断处理
- 与 Chaabi 安全处理器通信

**相关错误信息**:
```
ERR! SCU DLT expired
SCU IPC: 0x%x  0x%x
PSH Cold Reset By SCU
PSH_PM ERROR: PSH and SCU waiting for each other
```

### B. Chaabi 安全处理器
Intel 的硬件安全模块，负责:
- 安全启动验证
- 密钥管理
- DRM (HDCP 1.4/2.x)
- OTP (一次性可编程) 熔丝读取

**Chaabi 分区 (CH01-CH17)**:
```
RT: Reading partial Chaabi FW for CH01..
RT: Reading partial Chaabi FW for CH02..
RT: Reading Chaabi Token from eMMC..
RT: Reading Chaabi Token from UMIP in eMMC..
ERR! Total Chaabi Image Size > Chaabi IMR
```

### C. PSH (Platform Sensor Hub)
传感器处理器，运行独立固件:
```
PSH KERNEL VERSION: %X
PSH miaHOB version: %s.%s.%s.%x
******* PSH loader *******
l3gd20h_gyro  - 陀螺仪驱动
gyro_drdy     - 陀螺仪数据就绪中断
gyro_int      - 陀螺仪中断
```

### D. IAFW (IA Firmware)
x86 启动固件，位于 `0x3a080`:
- 负责初始化 x86 核心
- 设置内存映射
- 跳转到 Bootstub

---

## 5. 内存区域配置 (IMR)

IFWI 定义了多个 Isolated Memory Regions:

```
IMRN 配置 @ 0x103e8
├── Chaabi IMR      - 安全处理器专用内存
├── VXE IMR         - 视频编解码器内存
├── SPS IMR         - 信号处理内存
├── Xen IMR         - 虚拟化隔离区 (如果启用)
└── PSH IMR         - 传感器处理器内存

相关错误:
FATAL ERROR: TOC size is too large for IMR
FATAL ERROR: VXE FW image size is too large for IMR
FATAL ERROR: SPS image size is too large for IMR
FATAL ERROR: Xen image size is too large for IMR
```

---

## 6. 显示/HDMI 配置

从固件字符串提取的显示面板信息:
```
PANEL_JDI_CMD   - JDI 命令模式面板
PANEL_SHARP_CMD - Sharp 命令模式面板
PANEL_SHARP_VIDE - Sharp 视频模式面板
PANEL_AUO       - AUO 面板
PANEL_JDI_VID   - JDI 视频模式面板
```

HDMI 相关:
```
hdmi-i2c-gpio
hdmi-audio
HDMI_HPD (热插拔检测)
HDMI_LS_EN (电平转换使能)
hdmi_5v_en
LCD_PWM_SOC
HDCP1.4 device is a repeater. Not supported!
```

---

## 7. eMMC/存储配置

```
RT: Read SMIP from eMMC..
RT: Using eMMC/SDIO tuning defaults
RT: Using eMMC/SDIO tuning values from XML
WARNING: eMMC DLL failed to lock!
WARNING: eMMC DLL for eMMC 5.0 failed to lock!
WRN! Cannot disable Boot partition write protection!
WRN! Boot partition not write protected!
```

---

## 8. 安全特性

### OTP 熔丝
```
ERR! Fuse interrupt
ERR! Invalid stepping ID fuses
OTP Base Version: 0x...
OTP Customer Version: 0x...
Timed out waiting for OTP Power-ON
```

### 签名验证
每个主要固件分区都包含 RSA 签名:
- `MIPNS` (MIP Non-Secure Signed)
- `MIPDS` (MIP Data Signed)
- `IAFWS` (IA Firmware Signed)

**版本控制**:
```
Payload Base Version: 0x...
Payload Customer Version: 0x...
Comparing Base Versions AND Customer Versions
ERROR! Payload contains no NVM version section
```

---

## 9. 启动流程

```
┌──────────────────────────────────────────────────────────────────┐
│                Intel Moorefield IFWI Boot Flow                   │
└──────────────────────────────────────────────────────────────────┘

  硬件复位
     │
     ▼
┌─────────────┐
│  Boot ROM   │ ─── 从 SPI NOR 加载 UMIP
│  (On-chip)  │
└─────────────┘
     │
     ▼
┌─────────────┐
│    SCU      │ ─── 初始化 PMIC, 时钟
│  Firmware   │     读取 SMIP 配置
└─────────────┘
     │
     ▼
┌─────────────┐
│   Chaabi    │ ─── 安全验证 (CH00-CH17)
│  Security   │     解密 / 验签
└─────────────┘
     │
     ▼
┌─────────────┐
│    MRC      │ ─── 初始化 DDR 内存
│ (Memory)    │
└─────────────┘
     │
     ▼
┌─────────────┐
│    IAFW     │ ─── x86 核心初始化
│  (x86 FW)   │     设置 E820 内存映射
└─────────────┘
     │
     ▼
┌─────────────┐
│  Bootstub   │ ─── 加载 boot.img (kernel+ramdisk)
│  (second)   │     跳转到 Linux 内核
└─────────────┘
     │
     ▼
┌─────────────┐
│   Linux     │ ─── 64-bit Long Mode
│   Kernel    │
└─────────────┘
```

---

## 10. 与 boot.img 的关系

IFWI 中的 **Bootstub/IAFW** 组件负责:

1. **读取 GPT 分区表** 从 eMMC
2. **定位 boot 分区** (或 recovery/droidboot)
3. **解析 Android Boot Image** 头部 (`ANDROID!`)
4. **提取 kernel + ramdisk + second**
5. **执行 second (Bootstub)** 做最终准备
6. **跳转到 Linux 内核入口点**

因此，你的 `boot.img` 确实是由 IFWI 中的固件组件引导的。

---

## 11. 安全启动限制

Nokia N1 的 IFWI 包含:
- **Secure Boot** 强制签名验证
- **Anti-rollback** 版本回滚保护
- **Chaabi** 硬件安全模块

这意味着:
1. 无法刷入未签名的 IFWI
2. 无法降级固件版本
3. `boot.img` 必须符合 OEM 签名或使用 userdebug 固件

---

## 12. 提取的关键文件

在分析过程中发现的嵌入文件:
```
post.bin (gzip 压缩)
├── 偏移: 0x42014
├── 原始大小: 75,921 字节
└── 时间戳: 2015-05-25 02:06:43
```

---

## 13. 调试/开发信息

### 开发路径泄露
```
D:/WindRiver/viper-2.0/extAPI/iap/config/extra_kernel_add_ons/midas/pc_mgr.c
D:/WindRiver/viper-2.0/extAPI/iap/config/extra_kernel_add_ons/midas/syspm.c
D:/WindRiver/viper-2.0/extAPI/iap/config/extra_kernel_add_ons/midas/mrfd_dma.c
```

这表明 Intel 使用 **Wind River VxWorks** 作为 SCU/PSH 的实时操作系统基础。

### Intel 联系信息
```
And send it to <umg_emul@intel.com>, with subject including the string 'TNG' or 'ANN'.
```

---

## 结论

Nokia N1 的 IFWI (E123.2001) 是一个完整的 Intel Moorefield 平台固件，包含:

1. **多层安全机制** - Chaabi 硬件安全，签名验证
2. **完整的启动链** - 从 Boot ROM 到 Linux 内核
3. **丰富的外设支持** - 显示、HDMI、传感器、调制解调器
4. **严格的版本控制** - 防回滚保护

对于自定义 bootloader 开发 (如 LK 移植)，需要:
- 使用 userdebug/unlocked IFWI
- 或通过现有的 boot.img 加载机制
- 理解 Bootstub 传递给内核的 `boot_params` 结构
