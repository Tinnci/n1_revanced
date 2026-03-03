# Nokia N1 内核源码适配分析

## 概述

Nokia N1 与 ASUS ZenFone 2 共享 Intel Atom Z3580 (Moorefield) SoC，但外围硬件存在差异。
本文档详细分析了使用 CyanogenMod/android_kernel_asus_moorefield (cm-14.1) 源码为 Nokia N1 编译内核所需的全部修改。

**源码**: `CyanogenMod/android_kernel_asus_moorefield` cm-14.1 分支 (3.10.72)
**设备**: Nokia N1 (运行内核 3.10.62-x86_64_moor)

---

## 1. Nokia N1 完整硬件清单

### 1.1 SFI 设备表 (从 dmesg 提取)

设备通过 IAFW 固件的 SFI DEVS 表注册。以下是 Nokia N1 上所有检测到的设备：

#### IPC 总线设备

| 名称 | IRQ | 描述 |
|------|-----|------|
| soc_thrm | 0x01 | SoC 温度传感器 |
| bcove_adc | 0x32 | BasinCove ADC |
| bcove_bcu | 0x33 | BasinCove BCU (过流保护) |
| scove_thrm | 0x34 | ShadyCove PMIC 温度 |
| scove_power_btn | 0x1e | ShadyCove 电源按钮 |
| pmic_ccsm | 0x1b | PMIC 充电管理 |
| i2c_pmic_adap | 0x1b | PMIC I2C 适配器 |
| msic_gpio | 0x31 | MSIC GPIO |
| mrfld_wm8958 | 0xff | WM8958 音频 (RPMSG IPC) |
| scale_adc | 0xff | 缩放 ADC |

#### I2C 总线设备

| 总线 | 地址 | 名称 | 描述 |
|------|------|------|------|
| 1 | 0x1a | wm8958 | Wolfson WM8958 音频编解码器 |
| 2 | 0x36 | max17050 | Maxim MAX17050 电池电量计 |
| 4 | 0x53 | lm3559 | TI LM3559 相机闪光灯 |
| 4 | 0x10 | ov8858 | OmniVision OV8858 后摄 (8MP) |
| 4 | 0x36 | ov5693 | OmniVision OV5693 前摄 (5MP) |
| 6 | 0x5d | **Goodix-TS** | **Goodix GT928 触摸控制器** |
| 8 | 0x6b | bq24261_charger | TI BQ24261 充电芯片 |

#### 其他总线

| 总线 | 名称 | 描述 |
|------|------|------|
| SPI bus=5 | spi_max3111 | Maxim MAX3111 SPI-UART |
| SDIO bus=1 | bcm43xx_clk_vmmc | Broadcom BCM43xx WiFi |
| HSU bus=1 | bcm47521 port=0 | Broadcom BCM47521 GPS |

### 1.2 关键硬件参数

| 组件 | Nokia N1 | 详情 |
|------|----------|------|
| **SoC** | Intel Z3580 (Moorefield/Anniedale) | 4核 Silvermont, 64位 |
| **PMIC** | ShadyCove (PMIC-ID: 2) | BasinCove GPADC |
| **显示面板** | JDI_7x12_VID (PanelID=11) | 1536×2048, MIPI DSI 视频模式 |
| **触摸** | Goodix GT928 | I2C bus 6 @ 0x5D, IRQ GPIO 183, 10点触控 |
| **电池** | MAX17050 电量计 + BQ24261 充电 | MAX17047/50 兼容 |
| **音频** | WM8958 (Wolfson) | I2C bus 1 @ 0x1a, revision B |
| **后摄** | OV8858 | I2C bus 4 @ 0x10, 8MP |
| **前摄** | OV5693 | I2C bus 4 @ 0x36, 5MP |
| **闪光灯** | LM3559 | I2C bus 4 @ 0x53 |
| **WiFi** | **BCM43241 B4** (SDIO) | 26MHz 参考时钟, GPIO 64(中断)/96(使能) |
| **GPS** | BCM47521 | HSU port 0, UART 1 |
| **蓝牙** | BCM (via HSU) | GPIO 71(复位)/184(唤醒)/185(UART使能) |
| **eMMC** | 032GE4 29.1 GiB | HS200, SDHCI PCI 8086:1490 |
| **USB** | DWC3 OTG | PCI 0000:00:11.0, VID=8087 PID=0a5d |
| **传感器** | 通过 PSH (I2C bus 8) | LSM303DLHC(加速度/磁力), L3GD20H(陀螺), APDS-9900(光), LPS331AP(气压) |
| **NFC** | **PN544** (NXP) | I2C bus 5 @ 0x28, GPIO 174(中断)/175(使能)/176(复位) |

---

## 2. 硬件差异对比

### ZenFone 2 vs Nokia N1

| 组件 | ZenFone 2 (defconfig) | Nokia N1 (实际) | 兼容性 | 所需修改 |
|------|----------------------|----------------|--------|----------|
| **触摸控制器** | R69001 (Synaptics) `=y` | **Goodix GT928** | ❌ 不同 | 替换驱动 |
| **电池电量计** | MAX17042 `not set` | MAX17050 | ⚠️ 驱动集禁用 | 启用 CONFIG |
| **充电芯片** | BQ24261 `not set` | BQ24261 | ⚠️ 驱动禁用 | 启用 CONFIG |
| **前置摄像头** | OV5693 `not set` | OV5693 | ⚠️ 驱动禁用 | 启用 CONFIG |
| **后置摄像头** | OV8858 `=m` ✅ | OV8858 | ✅ 匹配 | 无需修改 |
| **闪光灯** | LM3559 `=m` ✅ | LM3559 | ✅ 匹配 | 无需修改 |
| **音频编解码器** | WM8958/WM8994 `=y` ✅ | WM8958 | ✅ 匹配 | 无需修改 |
| **PMIC** | ShadyCove `=y` ✅ | ShadyCove | ✅ 匹配 | 无需修改 |
| **WiFi** | BCMDHD (SDIO) `=m` ✅ | BCM43241 B4 (SDIO) | ✅ 匹配 | 固件路径需调整 |
| **GPS** | BCM47521 ✅ | BCM47521 | ✅ 匹配 | 无需修改 |
| **蓝牙** | BCM_BT_LPM `=m` ✅ | BCM BT | ✅ 匹配 | 无需修改 |
| **显示面板** | JDI_7x12_VID ✅ | JDI_7x12_VID | ✅ 匹配 | 无需修改 |
| **GPU/DRM** | tngdisp.ko ✅ | tngdisp.ko | ✅ 匹配 | 无需修改 |
| **IMX 摄像头** | IMX `=m` | ❌ 无此硬件 | N/A | 可禁用 |
| **NFC** | BCM20795 (SFI 表中) | **PN544** (I2C bus 5) | ⚠️ 不同芯片 | 需要不同驱动 (CONFIG_PN544_NFC) |

**结论**: 核心 SoC、PMIC、显示、音频、GPS 完全兼容。差异集中在 **触摸控制器**、**NFC 芯片型号**、部分 **电源管理/摄像头** 驱动的 defconfig 开关，以及 **WiFi 固件路径**。

---

## 3. 必须修改的文件

### 3.1 defconfig 修改 (`arch/x86/configs/x86_64_moor_defconfig`)

```diff
# === 触摸控制器：禁用 R69001，启用 Goodix GT9xx ===
- CONFIG_TOUCHSCREEN_R69001_I2C=y
+ # CONFIG_TOUCHSCREEN_R69001_I2C is not set
- # CONFIG_TOUCHSCREEN_GOODIX_GT9XX is not set
+ CONFIG_TOUCHSCREEN_GOODIX_GT9XX=y

# === 电池管理：启用 MAX17042/MAX17050 和 BQ24261 ===
- # CONFIG_BATTERY_MAX17042 is not set
+ CONFIG_BATTERY_MAX17042=y
- # CONFIG_BQ24261_CHARGER is not set
+ CONFIG_BQ24261_CHARGER=y

# === 前置摄像头：启用 OV5693 ===
- # CONFIG_VIDEO_OV5693 is not set
+ CONFIG_VIDEO_OV5693=m

# === 可选：禁用 ZenFone 2 专用的 IMX 摄像头 ===
- CONFIG_VIDEO_IMX=m
+ # CONFIG_VIDEO_IMX is not set

# === 推荐：启用内核配置导出（用于调试） ===
- # CONFIG_IKCONFIG is not set
+ CONFIG_IKCONFIG=y
+ CONFIG_IKCONFIG_PROC=y

# === 推荐：禁用模块强制签名（允许加载自编译模块） ===
- CONFIG_MODULE_SIG_FORCE=y
+ # CONFIG_MODULE_SIG_FORCE is not set

# === WiFi 固件路径：匹配 Nokia N1 的 BCM43241 B4 ===
- CONFIG_BCMDHD_FW_PATH="/system/etc/firmware/fw_bcmdhd.bin"
+ CONFIG_BCMDHD_FW_PATH="/system/etc/firmware/fw_bcmdhd.bin_43241_b4"
- CONFIG_BCMDHD_NVRAM_PATH="/system/etc/firmware/bcmdhd_aob.cal"
+ CONFIG_BCMDHD_NVRAM_PATH="/system/etc/firmware/bcmdhd_aob.cal_43241_b4"

# === NFC：启用 PN544 驱动 ===
+ CONFIG_PN544_NFC=m
+ CONFIG_NFC_PN544=m
+ CONFIG_NFC_PN544_I2C=m

# === 可选：版本调整（如需兼容原始 tngdisp.ko / 其他原始模块） ===
# 需修改 Makefile: SUBLEVEL = 72 → 62
# vermagic 将变为: 3.10.62-x86_64_moor SMP preempt mod_unload
# 这样可以加载设备上全部 36 个原始 .ko 模块
```

### 3.2 board.c 修改 (`arch/x86/platform/intel-mid/board.c`)

需要在 `sfi_device_table` 中添加 Goodix 触摸控制器的 SFI 匹配条目：

```c
// 在 sfi_device_table 数组中添加（Touch 部分附近）:
#ifdef CONFIG_TOUCHSCREEN_GOODIX_GT9XX
        {"Goodix-TS", SFI_DEV_TYPE_I2C, 0, &no_platform_data, NULL},
#endif
```

**说明**: Nokia N1 的 IAFW 固件 SFI DEVS 表中 **不包含** "Goodix-TS" 条目（详见 HARDWARE_PARAMETERS.md §2.2 和 §12.2）。
原始 Nokia N1 内核通过驱动代码自行注册 I2C 设备，绕过了 SFI 设备匹配机制。

CyanogenMod 源码同样没有此条目。添加此行后，内核会在 SFI 解析阶段在 I2C bus 6 创建对应的 I2C client 设备 (地址 0x5D 由 IAFW 运行时分配)，Goodix 驱动即可通过标准 I2C 匹配探测。

**替代方案**: 如果不修改 board.c，可在 GT9xx 驱动的 probe 函数中硬编码 `i2c_new_device()` 调用：
```c
// 在 gt9xx.c 中添加平台初始化函数
static struct i2c_board_info goodix_board_info = {
    I2C_BOARD_INFO("Goodix-TS", 0x5d),
    .irq = 183 + 256,  // GPIO 183 + INTEL_MID_IRQ_OFFSET
};
// 在 module_init 中: i2c_new_device(i2c_get_adapter(6), &goodix_board_info);
```

### 3.3 GT9xx 驱动修改 (`drivers/external_drivers/drivers/input/touchscreen/gt9xx/`)

#### 3.3.1 驱动名称修正 (`gt9xx.h`)

```diff
- #define GTP_I2C_NAME          "Goodix_TS"
+ #define GTP_I2C_NAME          "Goodix-TS"
```

**原因**: Nokia N1 IAFW 注册的设备名为 "Goodix-TS"（带连字符），而源码中为 "Goodix_TS"（带下划线）。I2C 匹配使用 `strcmp()`，必须完全一致。

#### 3.3.2 I2C 地址修正 (`gt9xx.c`)

```diff
  static s8 gtp_i2c_test(struct i2c_client *client)
  {
-     client->addr = 0x14;
+     // Nokia N1 使用 0x5D，不覆盖 SFI 提供的地址
+     // client->addr = 0x14;
```

**原因**: ZenFone 2 的 Goodix 使用 I2C 地址 0x14，但 Nokia N1 使用 **0x5D**。源码在 probe 时强制覆盖为 0x14，会导致 Nokia N1 上的触摸失效。

#### 3.3.3 GPIO 引脚配置 (`gt9xx.h`)

```diff
- #define GTP_RST_PORT    128    // ZenFone 2 触摸重置 GPIO
+ #define GTP_RST_PORT    191    // Nokia N1 触摸重置 GPIO (已通过 debugfs 确认)

- #define GTP_INT_PORT    133    // ZenFone 2 触摸中断 GPIO
+ #define GTP_INT_PORT    183    // Nokia N1 触摸中断 GPIO (IRQ 439 = GPIO 183 + INTEL_MID_IRQ_OFFSET 256)
```

**验证方法**: 通过 `mount -t debugfs debugfs /sys/kernel/debug && cat /sys/kernel/debug/gpio` 获取实际 GPIO 分配:
```
gpio-183 (GTP_INT_IRQ         ) in  hi    ← 中断引脚 (GPIO 183, IRQ = 183 + 256 = 439)
gpio-191 (GTP_RST_PORT        ) in  hi    ← 重置引脚 (GPIO 191)
```

**GPIO 编号系统**:
- LNW GPIO chip: base=0, count=192 (PCI 0000:00:0c.0)
- MSIC GPIO chip: base=192, count=8 (PMIC)
- IRQ = GPIO + `INTEL_MID_IRQ_OFFSET` (0x100 = 256)
- `/proc/interrupts` 中 `Goodix-TS` 的 IRQ=439 对应 GPIO=183

### 3.4 Makefile 版本修改 (可选)

如果需要加载设备上现有的 tngdisp.ko（而非重新编译），需要匹配 vermagic：

```diff
# 根目录 Makefile
- SUBLEVEL = 72
+ SUBLEVEL = 62
```

这样编译出的内核 vermagic 将为 `3.10.62-x86_64_moor SMP preempt mod_unload`，与原始 tngdisp.ko 完全匹配。

---

## 4. 不需要修改的部分

以下硬件在 ZenFone 2 源码中已完全支持，Nokia N1 可直接使用：

| 组件 | 源码位置 | 说明 |
|------|---------|------|
| 显示面板 (JDI_7x12_VID) | `drivers/external_drivers/intel_media/display/tng/drv/jdi_vid.c` | PanelID=11 已在 panel_handler 中注册 |
| tngdisp GPU/DRM | `drivers/external_drivers/intel_media/` | 218+ .c 文件, 完整 PVR RGX 源码 |
| ShadyCove PMIC | `CONFIG_REGULATOR_PMIC_SHADY_COVE=y` | 已默认启用 |
| WM8958 音频 | `CONFIG_SND_MOOR_MACHINE=m`, `CONFIG_MFD_WM8994=y` | 已默认启用 |
| BCMDHD WiFi | `CONFIG_BCMDHD=m`, `CONFIG_BCMDHD_SDIO=y` | 通用 SDIO 驱动 (需调整固件路径至 BCM43241 B4) |
| BCM BT | `CONFIG_BCM_BT_LPM=m` | 已默认启用 |
| DWC3 USB | 内置 | Z3580 通用 |
| PSH 传感器 | 内置 | 通过 IPC 通信，无需单独 I2C 设备 |
| sep54 安全引擎 | `drivers/staging/sep54/` | 内置 |
| eMMC (SDHCI) | `CONFIG_MMC=y`, `CONFIG_SDHCI_PCI=y` | 通用 PCI 设备 |
| Intel MID 平台 | `arch/x86/platform/intel-mid/` | 完整 SFI/IPC/SCU 支持 |

---

## 5. 编译验证检查清单

### 首次启动最低要求

- [ ] defconfig 修改完成 (触摸、电池、充电、前摄)
- [ ] board.c 添加 Goodix-TS SFI 条目
- [ ] GT9xx 驱动名称修正 ("Goodix-TS")
- [ ] GT9xx I2C 地址不再被强制覆盖
- [ ] Makefile SUBLEVEL 调整 (如需加载原始模块)
- [ ] MODULE_SIG_FORCE 禁用

### 启动后验证

- [ ] dmesg 中出现 `GTP driver installing`
- [ ] `GTP I2C Address: 0x5d`
- [ ] `input: goodix-ts as /devices/virtual/input/inputX`
- [ ] tngdisp.ko 正常加载 (显示输出正常)
- [ ] `max170xx_battery` 探测成功
- [ ] `bq24261_charger` 探测成功
- [ ] `/proc/config.gz` 可读 (IKCONFIG)

### 后续验证

- [ ] WiFi 可连接 (bcmdhd 模块 + BCM43241 B4 固件)
- [ ] 音频播放/录音正常 (WM8958)
- [ ] 摄像头预览 (OV8858 + OV5693)
- [ ] USB OTG 功能
- [ ] 充电功能正常
- [ ] 传感器数据 (PSH → LSM303DLHC, L3GD20H, APDS-9900, LPS331AP)
- [ ] NFC 通信 (PN544, I2C bus 5 @ 0x28)

---

## 6. 风险评估

### 高风险

| 风险 | 描述 | 缓解措施 |
|------|------|---------|
| vermagic 不匹配 | 编译出的内核无法加载原始 tngdisp.ko → 无屏幕 | 修改 SUBLEVEL=62 或重编译 tngdisp |
| Camera 子系统差异 | OV5693 平台数据 (GPIO, 时钟) 可能不同 | 从 dmesg 观测 GPIO 分配 |

### 中风险

| 风险 | 描述 | 缓解措施 |
|------|------|---------|
| 电池温度读取失败 | dmesg 已显示 `BP Temp read error:-34` | 检查 MAX17050 NTC 热敏电阻配置 |
| Intel regulator 探测失败 | 8 个 intel_regulator 探测失败 (error -22) | 不影响基本功能，稍后调查 |
| WiFi 固件路径 | defconfig 需指定 BCM43241 B4 固件路径 `/system/etc/firmware/fw_bcmdhd.bin_43241_b4` | 已添加到 defconfig 修改 |

### 低风险

| 风险 | 描述 | 缓解措施 |
|------|------|---------|
| NFC 芯片不同 | ZenFone 2 用 BCM20795，Nokia N1 用 PN544，需要不同驱动 | 启用 CONFIG_PN544_NFC，需验证 I2C 通信 |
| IMX 摄像头 | ZenFone 2 的 IMX 驱动不会影响 Nokia N1 | 可安全禁用 |
| eMMC 型号差异 | Nokia N1 使用 032GE4，无特殊要求 | 通用 SDHCI 驱动兼容 |

---

## 7. SFI 设备表遗留分析

> 详细数据参见 [HARDWARE_PARAMETERS.md](HARDWARE_PARAMETERS.md) §2 和 §12

### 7.1 SFI DEVS 表结构

Nokia N1 的 IAFW 固件 SFI DEVS 表包含 **46 个条目**（1174 字节），每条目 25 字节：

```c
struct sfi_device_table_entry {  // include/linux/sfi.h:154
    u8  type;         // SFI_DEV_TYPE_IPC=2, I2C=1, SPI=0, etc.
    u8  host_num;     // 总线编号
    u16 addr;         // I2C 地址或 IPC 设备编号
    u8  irq;          // 中断号
    u32 max_freq;     // 最大频率 (Hz)
    char name[16];    // 设备名称 (ASCII)
};
```

### 7.2 ZenFone 2 遗留条目识别

SFI 表中包含 3 个 ZenFone 2 遗留设备，被 Nokia N1 实际设备覆盖：

| 条目# | 遗留设备 | 总线/地址 | Nokia N1 实际 | 处理方式 |
|--------|---------|-----------|-------------|---------|
| #3 | imx135 | I2C 4 @ 0x10 | **ov8858** (条目#44) | 同地址覆盖，IMX 驱动探测失败 |
| #13 | imx132 | I2C 4 @ 0x36 | **ov5693** (条目#45) | 同地址覆盖，IMX 驱动探测失败 |
| #8 | r69001-ts-i2c | I2C 7 @ 0x55 | **Goodix-TS** (不在表中) | 不同总线/地址，r69001 探测失败 |

OV8858/OV5693 条目在表尾（#44-45），为 Nokia N1 适配时追加。

### 7.3 Goodix-TS 不在 SFI 表中

这是最重要的发现：Goodix GT928 触摸控制器（I2C bus 6 @ 0x5D）**完全不在 SFI DEVS 表中**。

原始 Nokia N1 内核通过以下机制之一注册设备：
1. 内核驱动在 `module_init()` 中直接调用 `i2c_new_device()`
2. 自定义 board 文件中的平台初始化代码
3. GT9xx 驱动内部的设备发现机制

**编译新内核时**必须解决此问题，方案见 §3.2 (board.c 修改) 或 §3.3 (驱动内硬编码)。

---

## 8. 原始模块兼容性

> 详细清单参见 [STOCK_BOOT_ANALYSIS.md](STOCK_BOOT_ANALYSIS.md) §3.3

### 8.1 原始 .ko 模块概览

设备出厂 ramdisk 包含 36 个 signed 内核模块 (vermagic: `3.10.62-x86_64_moor SMP preempt mod_unload`)：

| 类别 | 数量 | 关键模块 | 编译策略 |
|------|------|---------|---------|
| 显示/GPU | 4 | tngdisp.ko (2.4MB) | ⚠️ 尽量复用原始模块 (闭源 PowerVR) |
| 音频 | 11 | snd-soc-florida.ko (3.6MB) | 可重新编译 (有完整源码) |
| 摄像头 | 9 | atomisp-css*.ko, ov8858, ov5693 | 可重新编译 |
| WiFi/BT | 3 | bcmdhd.ko (1.1MB) | 可重新编译 |
| 其他 | 9 | sep3_15.ko, mac80211.ko | 按需编译 |

### 8.2 关键模块加载顺序

来自 stock `init.mofd_v1.rc` 的模块加载序列：

```bash
# Phase 1: on init (最早加载)
insmod /lib/modules/tngdisp.ko          # 显示 (必须最先)

# Phase 2: on boot
insmod /lib/modules/arizona-i2c.ko      # 音频 I2C 接口
insmod /lib/modules/snd-soc-arizona.ko  # 音频 ASoC
insmod /lib/modules/snd-soc-wm-adsp.ko # WM DSP
insmod /lib/modules/snd-soc-wm-hubs.ko # WM 集线器
insmod /lib/modules/snd-soc-florida.ko  # Florida/WM8958 编解码器
insmod /lib/modules/snd-soc-wm8994.ko  # WM8994 ASoC
insmod /lib/modules/snd-soc-sst-platform.ko dpcm_enable=1 dfw_enable=1
insmod /lib/modules/snd-intel-sst.ko    # Intel SST 核心
insmod /lib/modules/snd-moor-dpcm-florida.ko
insmod /lib/modules/snd-merr-dpcm-wm8958.ko
insmod /lib/modules/cfg80211.ko         # WiFi 框架
insmod /lib/modules/bcmdhd.ko           # BCM43241 驱动

# Phase 3: on post-fs
insmod /lib/modules/rmi4.ko             # ← 不存在! 静默失败
```

### 8.3 vermagic 兼容策略

要复用原始 36 个 .ko 模块（特别是闭源的 tngdisp.ko），需要：

1. Makefile `SUBLEVEL = 62`（不是源码默认的 72）
2. `CONFIG_MODULE_SIG_FORCE` 禁用（原始模块用不同密钥签名）
3. 编译选项一致：`SMP preempt mod_unload`

原始模块已备份到仓库 `firmware/stock_modules/` 目录。

---

## 9. 修改总结

```
需要修改的文件     : 4 个
  arch/x86/configs/x86_64_moor_defconfig  — 12 项 CONFIG 修改
    (触摸, 电池, 充电, 前摄, IKCONFIG, MODULE_SIG, WiFi固件路径, NFC)
  arch/x86/platform/intel-mid/board.c     — 添加 1 个 SFI 条目 (Goodix-TS)
  drivers/.../gt9xx/gt9xx.h               — 修改驱动名称 + GPIO 引脚
  drivers/.../gt9xx/gt9xx.c               — 移除 I2C 地址覆盖

可选修改           : 1 个
  Makefile                                — SUBLEVEL 72→62 (兼容全部 36 个原始模块)

完全不需要修改     : SoC 平台代码、显示驱动、音频、WiFi/BT/GPS、PMIC、USB、安全引擎
```

**适配难度**: 🟢 低 — 所有硬件驱动源码完整可用，差异仅限于外围设备配置开关和触摸控制器参数。

### 交叉引用文档

| 文档 | 内容 |
|------|------|
| [HARDWARE_PARAMETERS.md](HARDWARE_PARAMETERS.md) | 完整 SFI 表 (46 DEVS + 70 GPIO)、PCI 设备、调压器、热传感器 |
| [STOCK_BOOT_ANALYSIS.md](STOCK_BOOT_ANALYSIS.md) | Stock ramdisk 分析、36 模块清单、init 脚本、ZenFone 2 遗留 |
| [KERNEL_ANALYSIS.md](KERNEL_ANALYSIS.md) | 内核 ELF 分析、kallsyms、闭源驱动提取 |
| [FIRMWARE_INVENTORY.md](FIRMWARE_INVENTORY.md) | /system 分区固件文件清单 (36 文件, ~33MB) |
