# Nokia N1 完整硬件参数手册

> 通过 SFI 固件表、sysfs、debugfs、/proc 实机提取。所有数据均来自设备实测。

## 1. SoC 与 CPU

| 参数 | 值 |
|------|-----|
| SoC | Intel Atom Z3580 (Moorefield) |
| 架构 | x86_64 (Silvermont) |
| 核心数 | 4 |
| 基础频率 | 1.33 GHz |
| 最大睿频 | 2.33 GHz |
| CPU family | 6, model 90, stepping 0 |
| L2 Cache | 1024 KB |
| 地址空间 | 36-bit 物理 / 48-bit 虚拟 |
| 微码版本 | 0x38 |
| CPU 特性 | VMX, AES-NI, SSE4.2, POPCNT, RDRAND |

## 2. SFI DEVS 设备表 (完整)

> 来自 `/sys/firmware/sfi/tables/DEVS`，共 46 条目，1174 字节

### 2.1 IPC 设备 (片上/PMIC)

| 名称 | IRQ | MaxFreq | 说明 |
|------|-----|---------|------|
| soc_thrm | 1 | 25 MHz | SoC 热管理 |
| bcove_gpio | 49 | 25 MHz | BasinCove GPIO |
| bcove_adc | 50 | 25 MHz | BasinCove ADC |
| bcove_bcu | 51 | 25 MHz | BasinCove BCU (欠压保护) |
| scove_thrm | 52 | 25 MHz | ShadyCove 热管理 |
| bcove_tmu | 23 | 25 MHz | BasinCove TMU (温度监控) |
| scove_power_btn | 30 | 25 MHz | 电源按钮 |
| pmic_ccsm | 27 | 25 MHz | PMIC 充电状态机 |
| i2c_pmic_adap | 27 | 25 MHz | PMIC I2C 适配器 (bus 8) |
| gpio-keys | n/a | 25 MHz | 音量按键 GPIO |
| hdmi-i2c-gpio | n/a | 25 MHz | HDMI I2C GPIO |
| switch-mid | n/a | 25 MHz | USB 开关 |
| intel_mid_pwm | n/a | 25 MHz | PWM 控制器 |
| intel_kpd_led | n/a | 25 MHz | 键盘 LED 控制 |
| msic_gpio | 49 | 25 MHz | MSIC GPIO 控制器 |
| mrfld_wm8958 | n/a | 550 kHz | WM8958 平台设备 |
| watchdog | 59 | 25 MHz | 看门狗定时器 |
| sw_fuel_gauge | n/a | 25 MHz | 软件电量计 |
| sw_fuel_gauge_ha | n/a | 25 MHz | 电量计 HA |
| scale_adc | n/a | 25 MHz | Scale ADC |

### 2.2 I2C 设备

| 总线 | 地址 | 名称 | 驱动 | 说明 |
|------|------|------|------|------|
| 1 | 0x1a | wm8958 | wm8994 | Wolfson 音频编解码器 |
| 2 | 0x36 | max17050 | max170xx_battery | 电量计 |
| 4 | 0x10 | imx135 | — | 相机 (ZenFone 2 遗留, 未使用) |
| 4 | 0x10 | ov8858 | — | **后摄 OV8858** (实际使用) |
| 4 | 0x36 | imx132 | — | 相机 (ZenFone 2 遗留, 未使用) |
| 4 | 0x36 | ov5693 | — | **前摄 OV5693** (实际使用) |
| 4 | 0x53 | lm3559 | — | LED 闪光灯驱动 |
| 5 | 0x28 | pn544 | — | **NFC 控制器** |
| 7 | 0x55 | r69001-ts-i2c | — | 触摸 (ZenFone 2 遗留, 未使用) |
| 8 | 0x19 | lsm303dlhc_acel | — | 加速度计 |
| 8 | 0x1e | lsm303dlhc_comp | — | 磁力计/指南针 |
| 8 | 0x28 | — | — | — |
| 8 | 0x39 | apds-9900_als | — | 环境光/接近传感器 |
| 8 | 0x54 | lps331ap_press | — | 气压计 |
| 8 | 0x68 | l3gd20h_gyro | — | 陀螺仪 |
| 8 | 0x6b | bq24261_charger | bq24261_charger | 充电器 IC |

> **注意**: Bus 8 = PMIC I2C 适配器 (非标准 DesignWare I2C)  
> **注意**: Bus 6 的 Goodix-TS (0x5d) 不在 SFI 表中，由驱动自行注册

### 2.3 实际注册的 I2C 设备

| 总线 | 地址 | 名称 | 驱动 | 状态 |
|------|------|------|------|------|
| 1 | 0x1a | wm8958 | wm8994 | ✅ 活跃 |
| 2 | 0x36 | max17050 | max170xx_battery | ✅ 活跃 |
| 6 | 0x5d | Goodix-TS | Goodix-TS | ✅ 活跃 |
| 8 | 0x6b | bq24261_charger | bq24261_charger | ✅ 活跃 |

> 传感器和摄像头在 Recovery 模式下未初始化

### 2.4 SPI 设备

| 总线 | CS | IRQ | MaxFreq | 名称 |
|------|-----|-----|---------|------|
| 5 | 1 | 41 | 3.125 MHz | spi_max3111 (SPI UART) |
| 5 | 0 | n/a | 25 MHz | spi_led |

### 2.5 其他设备

| 类型 | 总线 | 名称 | 说明 |
|------|------|------|------|
| HSI | 0 | hsi_edlp_modem | 高速串行 modem 接口 |
| SD | 1 | bcm43xx_clk_vmmc | BCM WiFi 时钟/电源 |
| SD | 1 | WLAN_FAST_IRQ | WiFi 快速中断 (IRQ 40) |
| UART | 1 | bcm47521 | GPS 模块 |
| MDM | 0 | PANEL_JDI_VID | JDI 面板标识 |
| MDM | 0 | XMM7160_CONF_6 | Modem 配置 |
| USB | 0 | ULPICAL_7F | USB ULPI 校准 |
| USB | 0 | UTMICAL_PEDE3TX0 | USB UTMI 校准 |

## 3. SFI GPIO 映射表 (完整 70 条目)

> 来自 `/sys/firmware/sfi/tables/GPIO`，共 2404 字节

### 3.1 摄像头 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 0 | camera0_sb1 | 后摄 Sideband 1 (电源控制) |
| 1 | camera0_sb2 | 后摄 Sideband 2 |
| 2 | CAM_0_AF_EN | 后摄自动对焦使能 |
| 3 | xenon_ready | 闪光灯就绪信号 |
| 4 | GP_FLASH_STROBE | 闪光灯频闪 |
| 5 | GP_FLASH_TORCH | 闪光灯手电筒模式 |
| 6 | GP_FLASH_RESET | 闪光灯复位 |
| 7 | camera_1_power | 前摄电源 |
| 8 | camera_2_power | 辅助摄像头电源 (OV680) |
| 9 | camera_0_reset | 后摄复位 (OV8858) |
| 10 | camera_1_reset | 前摄复位 (OV5693) |

### 3.2 显示 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 16 | HDMI_HPD | HDMI 热插拔检测 |
| 17 | hdmi_i2c_scl | HDMI I2C 时钟 |
| 18 | hdmi_i2c_sda | HDMI I2C 数据 |
| 66 | disp_touch_int | 显示触摸中断 (ZenFone 2 遗留) |
| 68 | dsi_te1 | DSI Tearing Effect 信号 |
| 177 | HDMI_LS_EN | HDMI 电平转换使能 ★debugfs确认 |
| 183 | jdi_touch_int | **JDI 触摸中断 (Goodix GT928)** ★debugfs确认 |
| 189 | disp_bias_en | 显示偏压使能 |
| 190 | disp0_rst | 显示面板复位 |
| 191 | disp0_touch_rst | **触摸复位 (Goodix GT928)** ★debugfs确认 |

### 3.3 音频 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 14 | audiocodec_int | WM8958 中断 ★debugfs确认 |
| 54 | audiocodec_rst | WM8958 复位 / speaker boost 使能 |
| 67 | mute_enable | 静音使能 |
| 167 | audiocodec_jack | 耳机插孔检测 |

### 3.4 传感器 GPIO

| GPIO | 名称 | 对应芯片 |
|------|------|---------|
| 44 | als_int | APDS-9900 环境光/接近传感器中断 |
| 45 | compass_drdy | LSM303DLHC 磁力计数据就绪 |
| 46 | accel_int1 | LSM303DLHC 加速度计中断1 |
| 47 | accel_int2 | LSM303DLHC 加速度计中断2 |
| 48 | gyro_drdy | L3GD20H 陀螺仪数据就绪 |
| 49 | gyro_int | L3GD20H 陀螺仪中断 |
| 58 | baro_int1 | LPS331AP 气压计中断1 |
| 59 | baro_int2 | LPS331AP 气压计中断2 |

### 3.5 电池/充电 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 50 | battid_in | 电池 ID 输入 (tangier) |
| 51 | battid_out | 电池 ID 输出 (tangier) |
| 52 | max_fg_alert | MAX17050 电量计警报 |
| 53 | syst_burst_dis | 系统 burst 禁用 |
| 196 | battidin | 电池 ID 输入 (MSIC) |
| 197 | battidout | 电池 ID 输出 (MSIC) |

### 3.6 WiFi / Bluetooth GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 64 | WLAN-interrupt | BCM43241 WiFi 中断 |
| 71 | BT-reset | 蓝牙复位 |
| 96 | WLAN-enable | WiFi 使能 (电源控制) ★debugfs确认 |
| 184 | bt_wakeup | 蓝牙唤醒 |
| 185 | bt_uart_enable | 蓝牙 UART 使能 |

### 3.7 NFC GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 174 | NFC-intr | PN544 NFC 中断 |
| 175 | NFC-enable | NFC 使能 |
| 176 | NFC-reset | NFC 复位 |

### 3.8 用户输入 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 12 | haptics_en | 振动马达使能 |
| 13 | haptics_pwm | 振动马达 PWM |
| 61 | volume_down | 音量减 ★debugfs确认 |
| 62 | volume_up | 音量加 ★debugfs确认 |

### 3.9 Modem / HSIC GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 15 | hsi_cawake | HSI CA 唤醒 |
| 56 | modem_gpio1 | Modem GPIO 1 |
| 57 | modem_gpio2 | Modem GPIO 2 |
| 70 | usb_hsic_aux2 | HSIC 辅助2 |
| 162 | MODEM_CORE_DUMP | Modem 核心转储 |
| 168-172 | modem_jtag_* | Modem JTAG 调试 (TCK/TDO/TDI/TMS/TRST) |
| 173 | usb_hsic_aux1 | HSIC 辅助1 |
| 180 | ifx_mdm_rst_out | Modem 复位输出 |
| 181 | ifx_mdm_pwr_on | Modem 电源开启 |
| 182 | ifx_mdm_rst_pmu | Modem PMU 复位 |

### 3.10 MSIC GPIO

| GPIO | 控制器 | 名称 | 说明 |
|------|--------|------|------|
| 192 | msic_gpio | msic_gpio_base | MSIC GPIO 基地址 |
| 196 | msic_gpio | battidin | 电池 ID 输入 |
| 197 | msic_gpio | battidout | 电池 ID 输出 |
| 198 | msic_gpio | boardid0 | 板级 ID 0 |

### 3.11 其他 GPIO

| GPIO | 名称 | 说明 |
|------|------|------|
| 49 | ep_kbd_alrt | 键盘警报 (与 gyro_int 共享!) |
| 54 | spkr_boost_en | 扬声器增压使能 (与 audiocodec_rst 共享!) |
| 65 | max3111_int | SPI UART MAX3111 中断 |
| 165 | GPS-Enable | BCM47521 GPS 使能 ★debugfs确认 |

### 3.12 debugfs 实际 GPIO 状态

> 来自 `/sys/kernel/debug/gpio`，Recovery 模式下的活跃引脚

| GPIO | 标签 | 方向 | 值 | 说明 |
|------|------|------|-----|------|
| 14 | WM8994 IRQ | in | lo | 音频编解码器中断 |
| 16 | hdmi_hpd | in | lo | HDMI 未连接 |
| 17-18 | scl/sda | in | hi | HDMI I2C 总线 |
| 61 | volume_down | in | hi | 音量减（未按下） |
| 62 | volume_up | in | hi | 音量加（未按下） |
| 70 | hsic_wakeup | in | lo | HSIC 唤醒 |
| 77 | sd_cd | in | hi | SD 卡检测 |
| 96 | vwlan | out | lo | WiFi 电源（关闭） |
| 101 | gp_ulpi_0_clk | in | lo | ULPI 时钟 |
| 124-134 | hsu | — | — | 高速 UART 引脚 |
| 136-153 | gp_ulpi_0_* | in | lo | ULPI 数据总线 |
| 165 | intel_mid_gps_enable | out | lo | GPS 电源（关闭） |
| 173 | hsic_aux | in | lo | HSIC 辅助 |
| 177 | HDMI_LS_EN | in | lo | HDMI 电平转换（禁用） |
| **183** | **GTP_INT_IRQ** | **in** | **hi** | **Goodix 触摸中断** |
| **191** | **GTP_RST_PORT** | **in** | **hi** | **Goodix 触摸复位** |

## 4. 电源管理

### 4.1 电池

| 参数 | 值 |
|------|-----|
| 电池类型 | Li-ion |
| 设计容量 | 5300 mAh |
| 当前电压 | 4.154V |
| 当前温度 | 19°C |
| 电量计芯片 | MAX17050 (I2C bus 2, 0x36) |
| 充电芯片 | BQ24261 (I2C bus 8, 0x6b) |
| FG Alert GPIO | 52 |
| Battery ID GPIO | 50/51 (tangier), 196/197 (MSIC) |

### 4.2 调压器

| 编号 | 名称 | 电压 | 状态 | 用途 |
|------|------|------|------|------|
| 0 | regulator-dummy | — | — | 虚拟调压器 |
| 1 | VCC_1.8V_PDA | 1.8V | enabled | WM8958 LDO |
| 2 | V_BAT | 3.7V | enabled | 电池电压 |
| 3 | vprog1 | 2.8V | disabled | 摄像头电源 |
| 4 | vprog2 | 2.85V | disabled | 摄像头电源 |
| 5 | vprog3 | 2.9V | disabled | 摄像头电源 |
| 6 | vflex | 5.0V | disabled | 柔性供电 |
| 7 | vwlan | 1.8V | disabled | WiFi 电源 |
| 8 | AVDD1_3.0V | 3.0V | enabled | WM8958 模拟供电 |
| 9 | DCVDD_1.0V | 1.2V | enabled | WM8958 数字供电 |

### 4.3 热传感器

| Zone | 类型 | 温度 | 说明 |
|------|------|------|------|
| 0 | SoC_DTS0 | 27°C | SoC 数字热传感器 0 |
| 1 | SoC_DTS1 | 26°C | SoC 数字热传感器 1 |
| 2 | SYSTHERM0 | 23.3°C | 系统温度 0 |
| 3 | SYSTHERM1 | 22.2°C | 系统温度 1 |
| 4 | SYSTHERM2 | 20.2°C | 系统温度 2 |
| 5 | PMIC_DIE | 23.3°C | PMIC 芯片温度 |
| 6 | FrontSkin | — | 前面板温度（无读数） |
| 7 | BackSkin | — | 后面板温度（无读数） |
| 8 | max17047_battery | 19°C | 电池温度 |

## 5. 显示子系统

| 参数 | 值 |
|------|-----|
| 面板 | JDI_7x12_VID (PanelID=11) |
| 分辨率 | 1536 × 2048 |
| 色深 | 32bpp |
| 行跨度 | 6144 bytes |
| 接口 | MIPI DSI (Video Mode) |
| DRM 连接器 | DSI-1 (已连接), DVI-D-1 (HDMI) |
| 背光 | psb-bl, 最大亮度 255 |
| 背光当前 | 0 (Recovery 模式) |
| 显示复位 | GPIO 190 (disp0_rst) |
| 偏压使能 | GPIO 189 (disp_bias_en) |
| DSI TE | GPIO 68 (dsi_te1) |
| GPU | Intel PowerVR RGX (BVNC=1.72.4.12) |
| 显示 PCI | 8086:1480 + 8086:11a6 |

## 6. 输入设备

| Input | 名称 | Phys | 说明 |
|-------|------|------|------|
| input0 | gpio-keys | gpio-keys/input0 | 音量键 (GPIO 61/62) |
| input1 | mid_powerbtn | power-button/input0 | 电源键 (PMIC IRQ 30) |
| input2 | goodix-ts | input/ts | 触摸屏 (I2C6, 0x5d) |

## 7. PCI 设备映射

| BDF | VID:PID | Class | 说明 |
|-----|---------|-------|------|
| 00:00.0 | 8086:1470 | 0600 | 主桥 |
| 00:01.0 | 8086:1490 | 0805 | eMMC 控制器 |
| 00:01.2 | 8086:1491 | 0805 | SD 卡控制器 |
| 00:01.3 | 8086:1492 | 0805 | SDIO 控制器 (BCM43241 WiFi) |
| 00:02.0 | 8086:1480 | 0380 | 显示控制器 (PowerVR RGX) |
| 00:03.0 | 8086:1478 | 0480 | 视频编解码器 |
| 00:04.0-3 | 8086:1191 | 0700 | 高速 UART (HSU) ×4 |
| 00:05.0 | 8086:1192 | 0700 | HSU (GPS BCM47521) |
| 00:07.0-2 | 8086:1194 | 0880 | SPI 控制器 ×3 |
| 00:08.0-3 | 8086:1195 | 0780 | I2C 控制器 1-4 (DesignWare) |
| 00:09.0-2 | 8086:1196 | 0780 | I2C 控制器 5-7 (DesignWare) |
| 00:0a.0 | 8086:1197 | 0780 | 串行通信 |
| 00:0b.0 | 8086:1198 | 1080 | **SEP54 安全处理器** |
| 00:0c.0 | 8086:1199 | 0880 | **GPIO/FLIS 控制器** (192 pins) |
| 00:0d.0 | 8086:1495 | 0401 | 音频 DSP (SST, fw_sst_1495.bin) |
| 00:0e.0 | 8086:1496 | 0880 | DMA 控制器 |
| 00:10.0 | 8086:119d | 0c03 | **xHCI USB3 (DWC3)** |
| 00:11.0 | 8086:119e | 0c03 | xHCI USB3 Host |
| 00:12.0 | 8086:119f | ff00 | SCU IPC |
| 00:13.0 | 8086:11a0 | 0b40 | **PSH 传感器集线器** |
| 00:14.0 | 8086:11a1 | 0b40 | SCU |
| 00:15.0 | 8086:1497 | 0880 | PMC/OTG |
| 00:16.0 | 8086:11a3 | 0b40 | 协处理器 |
| 00:16.1 | 8086:11a4 | 0b40 | 协处理器 |
| 00:17.0 | 8086:1498 | 0880 | 平台配置 |
| 00:18.0 | 8086:11a6 | 0380 | 显示子设备 |
| 00:1a.0 | 8086:1499 | ff00 | 平台控制 |

## 8. I2C 总线适配器

| 总线 | PCI BDF | 名称 | 速率 | 挂载设备 |
|------|---------|------|------|---------|
| 1 | 00:08.0 | i2c-designware-1 | 550 kHz | WM8958 |
| 2 | 00:08.1 | i2c-designware-2 | 400 kHz | MAX17050 |
| 3 | 00:08.2 | i2c-designware-3 | 400 kHz | — |
| 4 | 00:08.3 | i2c-designware-4 | 400 kHz | OV8858, OV5693, LM3559 |
| 5 | 00:09.0 | i2c-designware-5 | 400 kHz | PN544 (NFC) |
| 6 | 00:09.1 | i2c-designware-6 | 400 kHz | Goodix GT928 |
| 7 | 00:09.2 | i2c-designware-7 | 400 kHz | (r69001 遗留) |
| 8 | — | PMIC I2C Adapter | 400 kHz | 传感器群, BQ24261 |
| 10 | — | i2c-gpio.10 | — | HDMI DDC |

## 9. 中断映射

### 9.1 关键 IRQ 分配

| IRQ | 类型 | 设备 | GPIO |
|-----|------|------|------|
| 27 | IO-APIC | i2c_pmic_adap, bq24261, pmic_ccsm | — |
| 33 | IO-APIC | sep54 | — |
| 34 | IO-APIC | dwc3 (USB3) | — |
| 47 | IO-APIC | intel_psh_ipc | — |
| 60 | wm8994_edge | wm8994 (音频编解码器) | GPIO 14 |
| 439 | LNW-GPIO-demux | **Goodix-TS** | **GPIO 183** |

### 9.2 GPIO → IRQ 转换

```
IRQ = GPIO + INTEL_MID_IRQ_OFFSET (256)
GPIO = IRQ - 256

示例:
  GPIO 183 → IRQ 439 (Goodix 触摸)
  GPIO 61  → IRQ 317 (音量减)
  GPIO 62  → IRQ 318 (音量加)
```

## 10. SFI OEM 表

### 10.1 OEMB 表 (96 字节)

| 偏移 | 值 | 说明 |
|------|-----|------|
| 0x18 | 0x09 | Board ID |
| 0x20 | 0x02 | PMIC ID (ShadyCove) |
| 0x21 | 0x13 | PMIC 修订版 |
| 0x38 | "L25110TYN1K3" | **设备序列号** |

### 10.2 SPID (实测)

```
vendor_id:           0000
manufacturer_id:     0000  
platform_family_id:  0000
hardware_id:         0008
product_line_id:     0000
customer_id:         0000
```

> hardware_id = 0x0008 标识 Nokia N1

### 10.3 Board Info

| 属性 | 值 |
|------|-----|
| ro.product.model | N1 |
| ro.product.board | moorefield |
| hardware_id | N1 |

## 11. 传感器芯片列表

> 所有传感器通过 PMIC I2C 适配器 (Bus 8) 连接，由 PSH 传感器集线器管理

| 芯片 | I2C 地址 | 类型 | GPIO 中断 |
|------|---------|------|----------|
| LSM303DLHC | 0x19 | 加速度计 | 46 (int1), 47 (int2) |
| LSM303DLHC | 0x1e | 磁力计/指南针 | 45 (drdy) |
| L3GD20H | 0x68 | 陀螺仪 | 48 (drdy), 49 (int) |
| APDS-9900 | 0x39 | 环境光/接近 | 44 (int) |
| LPS331AP | 0x54 | 气压计 | 58 (int1), 59 (int2) |

## 12. 关键发现与注意事项

### 12.1 SFI 表中的 ZenFone 2 遗留条目

SFI DEVS 表包含来自 ZenFone 2 的遗留设备条目，说明 Nokia N1 的 IFWI 固件基于 ZenFone 2：

| 遗留设备 | 实际设备 | 位置 |
|---------|---------|------|
| imx135 (bus 4, 0x10) | **ov8858** (bus 4, 0x10) | 后摄 |
| imx132 (bus 4, 0x36) | **ov5693** (bus 4, 0x36) | 前摄 |
| r69001-ts-i2c (bus 7, 0x55) | **Goodix-TS** (bus 6, 0x5d) | 触摸 |

OV8858/OV5693 条目在 SFI 表末尾（条目 44-45），为后追加。

### 12.2 Goodix 触摸不在 SFI 表中

Goodix-TS 虽然工作正常 (I2C bus 6, 0x5d, IRQ 439)，但不在 SFI DEVS 表中。
这意味着原始内核通过驱动代码直接注册，而非通过 SFI 设备匹配。

**对内核编译的影响**：
- 需要在 board.c 中添加 SFI 条目，或
- 在 GT9xx 驱动中硬编码 I2C 总线和地址

### 12.3 GPIO 复用冲突

两组 GPIO 共享同一引脚号：
- GPIO 49: gyro_int **和** ep_kbd_alrt
- GPIO 54: audiocodec_rst **和** spkr_boost_en

### 12.4 NFC 支持

设备具有 NXP PN544 NFC 控制器：
- I2C bus 5, 地址 0x28
- GPIO: 174 (中断), 175 (使能), 176 (复位)
- defconfig 需要 `CONFIG_PN544_NFC`

---

*文档生成时间: 基于设备实测数据，Recovery (TWRP) 模式下*  
*SFI 表二进制文件保存于: /tmp/sfi_gpio.bin, /tmp/sfi_devs.bin, /tmp/sfi_oem0.bin, /tmp/sfi_oemb.bin*
