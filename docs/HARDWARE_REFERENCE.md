# Nokia N1 硬件参数完整参考 (已验证)

**采集日期**: 2025-03-03  
**采集方式**: 设备实测 (ADB over USB)  
**状态**: TWRP Recovery 运行中

---

## 1. SoC / CPU

| 参数 | 值 |
|------|-----|
| SoC | Intel Atom Z3580 (Moorefield) |
| CPU 型号 | `Intel(R) Atom(TM) CPU Z3580 @ 1.33GHz` |
| 架构 | x86_64 (Silvermont 微架构) |
| CPU Family | 6, Model 90, Stepping 0 |
| 核心数 | 4 核心, 4 线程 |
| 最大频率 | 2333 MHz (Turbo) |
| L2 Cache | 1024 KB |
| 地址空间 | 36-bit 物理, 48-bit 虚拟 |
| 指令集特性 | SSE4.2, AES-NI, POPCNT, MOVBE, VMX, VT-x+EPT |
| Microcode | 0x38 |
| BogoMIPS | 2662.40 |

## 2. 内存

| 参数 | 值 |
|------|-----|
| 总容量 | 1994204 KB (~2 GB) |
| 类型 | LPDDR3 (推测) |
| ZRAM 大小 | 100 MB (zramsize=104857600) |

## 3. 存储 (eMMC)

| 参数 | 值 |
|------|-----|
| 设备名 | 032GE4 |
| 类型 | MMC |
| 厂商 ID | 0x000011 (Toshiba) |
| OEM ID | 0x0100 |
| CID | 1101003033324745340015a9f0b5b100 |
| CSD | d05e00320f5903ffffffffef92400000 |
| 总扇区数 | 61071360 (512B/sector → ~29.1 GB) |
| PCI 设备 | 0000:00:01.0 (Intel, ID=0x1490) |
| 驱动 | sdhci-pci |
| 块设备路径 | `/dev/block/pci/pci0000:00/0000:00:01.0/by-name/` |

## 4. GPT 分区表 (当前)

> **已修改** (2025-03-03): boot 分区从 16MB 扩展至 32MB，recovery 分区已删除（合并入 boot）。

| # | 名称 | 起始扇区 | 结束扇区 | 大小 | 状态 |
|---|------|---------|---------|------|------|
| 1 | **boot** | 34 | **65569** | **32 MB** | TWRP + 调试工具 |
| 2 | fastboot | 65570 | 98337 | 16 MB | Droidboot |
| 3 | splashscreen | 98338 | 106529 | 4 MB | 开机画面 |
| 4 | boot-one-shot | 106530 | 139297 | 16 MB | OTA 一次性启动 |
| 5 | ramdump | 139298 | 172065 | 16 MB | 空 (全零) |
| 6 | silentlake | 172066 | 176161 | 2 MB | Intel TXE |
| 7 | userkeystore | 176162 | 178209 | 1 MB | 密钥存储 |
| 8 | persistent | 178210 | 180257 | 1 MB | 持久数据 |
| 9 | reserved | 180258 | 524321 | 168 MB | 空 (全零) |
| 10 | panic | 524322 | 589857 | 32 MB | 内核崩溃 dump |
| 11 | factory | 589858 | 852001 | 128 MB | 出厂数据 |
| 12 | misc | 852008 | 1114151 | 128 MB | Bootloader 通信 |
| 13 | infork | 1114152 | 1116199 | 1 MB | — |
| 14 | config | 1116200 | 1378343 | 128 MB | 配置 |
| 15 | cache | 1378344 | 4524071 | 1.5 GB | 缓存 |
| 16 | logs | 4524072 | 4786215 | 128 MB | 日志 |
| 17 | system | 4786216 | 8980519 | 2 GB | Android 系统 |
| 18 | data | 8980520 | 61071326 | 24.8 GB | 用户数据 |

**GPT GUID**: b1f2f9ff-eeb2-6e36-bde7-e50ec048aa64  
**总扇区数**: 61,071,360 (512B/sector)  
**IAFW 分区查找**: 按 GPT 名称 (UTF-16LE) 查找，不硬编码扇区号

## 5. PCI 设备完整列表

| BDF | Vendor:Device | 类型 (Class) | 驱动 | 描述 |
|-----|--------------|-------------|------|------|
| 00:00.0 | 8086:1470 | 0x060000 (Host Bridge) | — | SoC 主桥 |
| 00:01.0 | 8086:1490 | 0x080501 (SD Host) | sdhci-pci | eMMC 控制器 (主存储) |
| 00:01.2 | 8086:1491 | 0x080501 (SD Host) | sdhci-pci | SD/SDIO 控制器 |
| 00:01.3 | 8086:1492 | 0x080501 (SD Host) | sdhci-pci | SD/SDIO 控制器 |
| 00:02.0 | 8086:1480 | 0x038000 (Display) | pvrsrvkm | **PowerVR G6430 GPU** |
| 00:03.0 | 8086:1478 | 0x048000 (Multimedia) | — | 视频编解码器 |
| 00:04.0-3 | 8086:1191 | 0x070002 (Serial) | HSU | 高速UART ×4 |
| 00:05.0 | 8086:1192 | 0x070002 (Serial) | HSU | 高速UART |
| 00:07.0-2 | 8086:1194 | 0x088000 (System) | ce4100_spi | SPI 控制器 ×3 |
| 00:08.0-3 | 8086:1195 | 0x078000 (I2C) | i2c-designware-pci | I2C 总线 ×4 |
| 00:09.0-2 | 8086:1196 | 0x078000 (I2C) | i2c-designware-pci | I2C 总线 ×3 |
| 00:0a.0 | 8086:1197 | 0x078000 | — | I2C (未绑定) |
| 00:0b.0 | 8086:1198 | 0x108000 | sep54 | 安全引擎 (SEP) |
| 00:0c.0 | 8086:1199 | 0x088000 | langwell_gpio | **GPIO 控制器** |
| 00:0d.0 | 8086:1495 | 0x040100 (Audio) | — | 音频控制器 |
| 00:0e.0 | 8086:1496 | 0x088000 | intel_mid_dma | DMA 控制器 |
| 00:10.0 | 8086:119d | 0x0c0320 (USB EHCI) | ehci-intel-hsic | USB HSIC 控制器 |
| 00:11.0 | 8086:119e | 0x0c0320 (USB) | dwc3_otg | **USB DWC3 OTG** (主USB) |
| 00:12.0 | 8086:119f | 0xff0000 | — | 未知 |
| 00:13.0 | 8086:11a0 | 0x0b4000 | intel_scu_ipc | **SCU IPC** (PMIC通信) |
| 00:14.0 | 8086:11a1 | 0x0b4000 | intel_pmu_driver | 电源管理单元 |
| 00:15.0 | 8086:1497 | 0x088000 | intel_mid_dma | DMA 控制器 |
| 00:16.0 | 8086:11a3 | 0x0b4000 | intel_psh_ipc | 传感器Hub IPC |
| 00:16.1 | 8086:11a4 | 0x0b4000 | psh | 传感器Hub |
| 00:17.0 | 8086:1498 | 0x088000 | — | 未知系统外设 |
| 00:18.0 | 8086:11a6 | 0x038000 (Display) | — | 第二显示控制器 |
| 00:1a.0 | 8086:1499 | 0xff0000 | — | 未知 |

## 6. 显示子系统

| 参数 | 值 |
|------|-----|
| GPU | PowerVR Rogue G6430 (PCI 00:02.0) |
| GPU Vendor:Device | 8086:1480 |
| GPU 驱动 | pvrsrvkm |
| DRM 驱动 | psb (v5.6.0.1202) |
| 显示接口 | DSI (MIPI DSI) |
| 面板 | JDI VID (Panel ID=11, dual-link) |
| 原生分辨率 | 1536×2048 (竖屏) / 2048×1536 (横屏) |
| 显示状态 | card0-DSI-1: connected, enabled, DPMS=On |
| DVI-D | card0-DVI-D-1: disconnected |
| 背光控制器 | psb-bl |
| 背光路径 | `/sys/class/backlight/psb-bl/brightness` |
| 最大亮度 | 255 |
| 当前亮度 | 0 (kexec后screen off) |
| GPU BAR0 | 0xC0000000 (32MB MMIO) |
| GPU BAR2 | 0x80000000 (512MB GTT) |
| 帧缓冲 | psbfb, BGRA8888 |

### 关键 GPIO (显示相关)

| GPIO | SFI 名称 | 功能 |
|------|---------|------|
| 189 | disp_bias_en | 面板偏压电源 |
| 190 | disp0_rst | 面板复位 |
| 191 | disp0_touch_rst | 触摸屏复位 |

### 显示驱动模块

- 文件: `/lib/modules/tngdisp.ko`
- 大小: 2,411,364 bytes
- 架构: ELF 64-bit x86_64
- 加载时机: early-init (在 recovery 启动前)

## 7. 输入设备

| 设备 | 名称 | 总线 | VID:PID | 事件节点 |
|------|------|------|---------|---------|
| 物理键 | gpio-keys | Platform | 0001:0001 | event0 |
| 电源键 | mid_powerbtn | Platform | 0000:0000 | event1 |
| 触摸屏 | goodix-ts | I2C (0x0018) | dead:beef | event2 |

**触摸屏**: Goodix (汇顶科技), I2C 接口, 支持多点触控

## 8. 电源管理

### 电池
| 参数 | 值 |
|------|-----|
| 电池芯片 | Maxim MAX17047 |
| 充电芯片 | TI BQ24261 |
| 电池状态 | Full (100%) |
| 电池温度 | 19°C |
| PMIC 类型 | Shadycove |
| PMIC NVM 版本 | 0x1C |

### 热管理

| Zone | 类型 | 温度 |
|------|------|------|
| thermal_zone0 | SoC_DTS0 (CPU核心0) | 27.0°C |
| thermal_zone1 | SoC_DTS1 (CPU核心1) | 25.0°C |
| thermal_zone2 | SYSTHERM0 (系统温度) | 22.9°C |
| thermal_zone3 | SYSTHERM1 | 21.9°C |
| thermal_zone4 | SYSTHERM2 | 19.7°C |
| thermal_zone5 | PMIC_DIE (PMIC芯片) | 23.0°C |
| thermal_zone6 | FrontSkin | N/A |
| thermal_zone7 | BackSkin | N/A |
| thermal_zone8 | Battery (MAX17047) | 19.0°C |

## 9. USB

| 参数 | 值 |
|------|-----|
| 控制器 | Intel DWC3 OTG (PCI 00:11.0, 8086:119e) |
| HSIC 控制器 | Intel EHCI HSIC (PCI 00:10.0, 8086:119d) |
| DWC3 MMIO Base | 0xF9100000 (BAR0) |
| DWC3 IP 版本 | 2.50a (GSNPSID=0x5533250A) |
| 当前 VID:PID | 18D1:4EE2 (Google MTP+ADB) |
| 当前功能 | mtp,ffs |
| Recovery VID:PID | 8087:0A5D (Intel ADB) |
| Fastboot VID:PID | 18D1:4EE0 (Google Fastboot) |

### DWC3 黄金值寄存器 (USB PHY 初始化必需)

| 偏移 | 寄存器名 | 抓取值 | 说明 |
|------|---------|--------|------|
| 0xC100 | GSBUSCFG0 | 0x00000006 | 总线配置 |
| 0xC104 | GSBUSCFG1 | 0x00000F00 | 总线突发传输配置 |
| 0xC110 | GCTL | 0x27102000 | PRTCAPDIR=2 (Device Mode) |
| 0xC200 | GUSB2PHYCFG | 0x00002600 | PHY 魔术值 (**必写**，否则 USB 不通) |
| 0xC2C0 | GUSB3PIPECTL | 0x02000002 | PIPE 控制 |

## 10. 固件版本

| 组件 | 版本 |
|------|------|
| IFWI | E123.2001 |
| IA32 FW | 0002.0013 |
| SCU FW | 00B0.0035 |
| SCU BS | 00B0.0001 |
| MIA | 00B0.3230 |
| Chaabi | 0064.0501 |
| Valhooks | 0002.0005 |
| P-Unit | 0000.0032 |
| Microcode | 0000.0038 |
| PMIC NVM | 0x1C |

## 11. 内核命令行 (完整)

```
init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 
watchdog.watchdog_thresh=60 androidboot.spid=0000:0000:0000:0008:0000:0000 
androidboot.serialno=L25110TYN1K3 gpt bootboost=1 androidboot.wakesrc=08 
androidboot.mode=main androidboot.bootreason=watchdog
```

| 参数 | 值 | 说明 |
|------|-----|------|
| init | /init | Android init 进程 |
| pci | noearly | 延迟PCI初始化 |
| loglevel | 0 | 抑制控制台输出 |
| vmalloc | 256M | 内核虚拟内存分配池 |
| androidboot.hardware | mofd_v1 | 硬件名称 (触发 init.recovery.mofd_v1.rc) |
| watchdog.watchdog_thresh | 60 | 看门狗超时60秒 |
| androidboot.spid | 0000:0000:0000:0008:0000:0000 | 系统产品ID |
| androidboot.serialno | L25110TYN1K3 | 设备序列号 |
| gpt | — | 使用GPT分区表 |
| bootboost | 1 | 启动加速 |
| androidboot.wakesrc | 08 | 唤醒源 |
| androidboot.mode | main | 主启动模式 |
| androidboot.bootreason | watchdog | 启动原因 |

## 12. SFI 固件表

设备使用 Intel SFI (Simple Firmware Interface) 而非 ACPI 或 Device Tree：

| 表名 | 说明 |
|------|------|
| APIC | 中断控制器 |
| CPUS | CPU 拓扑 |
| DEVS | 设备列表 |
| FREQ | CPU 频率表 |
| GPIO | GPIO 引脚映射 (含 disp_bias_en 等) |
| MCFG | PCI 配置空间 |
| MMAP | 内存映射 |
| OEM0 | OEM 自定义表 |
| OEMB | OEM 自定义表 B |
| SYST | 系统表 |
| WAKE | 唤醒源 |
| XSDT | 扩展系统表 |
