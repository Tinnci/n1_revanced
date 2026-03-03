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
| **WiFi** | BCM43xx (SDIO) | 26MHz 参考时钟, GPIO 64/96 |
| **GPS** | BCM47521 | HSU port 0 |
| **蓝牙** | BCM (via HSU) | ttyMFD0 |
| **eMMC** | 032GE4 29.1 GiB | HS200, SDHCI PCI 8086:1490 |
| **USB** | DWC3 OTG | PCI 0000:00:11.0 |
| **传感器** | 通过 PSH (Platform Sensor Hub) | 无独立 I2C 设备 |
| **NFC** | ❌ 无 | |

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
| **WiFi** | BCMDHD (SDIO) `=m` ✅ | BCM43xx (SDIO) | ✅ 匹配 | 无需修改 |
| **GPS** | BCM47521 ✅ | BCM47521 | ✅ 匹配 | 无需修改 |
| **蓝牙** | BCM_BT_LPM `=m` ✅ | BCM BT | ✅ 匹配 | 无需修改 |
| **显示面板** | JDI_7x12_VID ✅ | JDI_7x12_VID | ✅ 匹配 | 无需修改 |
| **GPU/DRM** | tngdisp.ko ✅ | tngdisp.ko | ✅ 匹配 | 无需修改 |
| **IMX 摄像头** | IMX `=m` | ❌ 无此硬件 | N/A | 可禁用 |
| **NFC** | BCM20795 (SFI 表中) | ❌ 无此硬件 | N/A | 无影响 |

**结论**: 核心 SoC、PMIC、显示、音频、WiFi/BT/GPS 完全兼容。差异集中在 **触摸控制器** 和部分 **电源管理/摄像头** 驱动的 defconfig 开关。

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

# === 可选：版本调整（如需兼容原始 tngdisp.ko） ===
# 需修改 Makefile: SUBLEVEL = 72 → 62
```

### 3.2 board.c 修改 (`arch/x86/platform/intel-mid/board.c`)

需要在 `sfi_device_table` 中添加 Goodix 触摸控制器的 SFI 匹配条目：

```c
// 在 sfi_device_table 数组中添加（Touch 部分附近）:
#ifdef CONFIG_TOUCHSCREEN_GOODIX_GT9XX
        {"Goodix-TS", SFI_DEV_TYPE_I2C, 0, &no_platform_data, NULL},
#endif
```

**说明**: Nokia N1 的 IAFW 固件 SFI DEVS 表中包含 "Goodix-TS" 设备条目。CyanogenMod 源码中默认没有此条目，因此 SFI 解析时会跳过它。添加此行后，内核会在 I2C bus 6 创建对应的 I2C client 设备，Goodix 驱动即可探测。

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
| BCMDHD WiFi | `CONFIG_BCMDHD=m`, `CONFIG_BCMDHD_SDIO=y` | 通用 SDIO 驱动 |
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

- [ ] WiFi 可连接 (bcmdhd 模块 + 固件)
- [ ] 音频播放/录音正常 (WM8958)
- [ ] 摄像头预览 (OV8858 + OV5693)
- [ ] USB OTG 功能
- [ ] 充电功能正常
- [ ] 传感器数据 (PSH)

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
| WiFi 固件路径 | defconfig 中路径 `/system/etc/firmware/fw_bcmdhd.bin` 需匹配设备实际路径 | 确认 /system 分区存在对应固件 |

### 低风险

| 风险 | 描述 | 缓解措施 |
|------|------|---------|
| NFC 缺位 | SFI 表中无 NFC 设备，源码中 NFC 驱动不会加载 | 无影响 |
| IMX 摄像头 | ZenFone 2 的 IMX 驱动不会影响 Nokia N1 | 可安全禁用 |
| eMMC 型号差异 | Nokia N1 使用 032GE4，无特殊要求 | 通用 SDHCI 驱动兼容 |

---

## 7. 修改总结

```
需要修改的文件     : 4 个
  arch/x86/configs/x86_64_moor_defconfig  — 8 项 CONFIG 修改
  arch/x86/platform/intel-mid/board.c     — 添加 1 个 SFI 条目
  drivers/.../gt9xx/gt9xx.h               — 修改驱动名称 + GPIO 引脚
  drivers/.../gt9xx/gt9xx.c               — 移除 I2C 地址覆盖

可选修改           : 2 个
  Makefile                                — SUBLEVEL 72→62 (兼容原始模块)
  scripts/config                          — 启用 IKCONFIG, 禁用 MODULE_SIG_FORCE

完全不需要修改     : SoC 平台代码、显示驱动、音频、WiFi/BT/GPS、PMIC、USB、安全引擎
```

**适配难度**: 🟢 低 — 所有硬件驱动源码完整可用，差异仅限于外围设备配置开关和触摸控制器参数。
