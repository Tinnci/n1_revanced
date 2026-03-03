# Nokia N1 Stock Boot 镜像分析

> 分析 `images/original/` 中的原始 boot 镜像，验证 TWRP 与原始内核的关系。

## 1. 镜像概览

### 1.1 文件信息

| 镜像 | 大小 | 说明 |
|------|------|------|
| stock_boot_cnb19.img | 10,765,620 bytes | 中国大陆版 (China Build 19) |
| stock_boot_fmb19.img | 10,765,620 bytes | 国际版 (Foreign Market Build 19) |

### 1.2 Boot Image 头部

两个镜像的头部参数一致：

| 参数 | 值 |
|------|-----|
| Magic | `ANDROID!` |
| Kernel size | 6,330,768 bytes (6.04 MB) |
| Kernel addr | 0x10008000 |
| Ramdisk addr | 0x11000000 |
| Second size | 8,192 bytes |
| Second addr | 0x10f00000 |
| Tags addr | 0x10000100 |
| Page size | 2,048 |
| Board name | (空) |

### 1.3 Cmdline 参数（Stock 原始）

```
init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1
watchdog.watchdog_thresh=60
androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
androidboot.serialno=01234567890123456789
gpt snd_pcm.maximum_substreams=8 panic=15
ip=50.0.0.2:50.0.0.1::255.255.255.0::usb0:on
debug_locks=0
n_gsm.mux_base_conf="ttyACM0,0 ttyXMM0,1"
bootboost=1
```

> 注：`xxxx:xxxx:xxxx:xxxx:xxxx:xxxx` 和 `01234567890123456789` 是占位符，由 bootstub 在启动时替换为真实值。

### 1.4 TWRP cmdline 对比

TWRP 精简了 cmdline，移除了以下参数：
- `snd_pcm.maximum_substreams=8` (音频 PCM 子流数)
- `ip=50.0.0.2:50.0.0.1::255.255.255.0::usb0:on` (USB 网络调试)
- `debug_locks=0` (调试锁)
- `n_gsm.mux_base_conf="ttyACM0,0 ttyXMM0,1"` (GSM modem 复用配置)

## 2. 组件哈希对比

### 2.1 内核

| 来源 | SHA256 |
|------|--------|
| stock_cnb19 | `b1b77448e80bc89d5893794b783a4300838d9657080558a7d68d4f3e6e7aca0e` |
| stock_fmb19 | `d8be6cfb5b77e5be72b477f103281e817aeaf684537c228da4288a35e8dc6711` |
| **TWRP kernel** | `d8be6cfb5b77e5be72b477f103281e817aeaf684537c228da4288a35e8dc6711` |

**结论：TWRP 内核 = stock_fmb19 内核（字节级完全相同）**

### 2.2 Second Bootloader (Bootstub)

| 来源 | SHA256 |
|------|--------|
| stock_cnb19 | `6eb5ac09b14165e2...` |
| stock_fmb19 | `6eb5ac09b14165e2...` |
| **TWRP second** | `6eb5ac09b14165e2...` |

**结论：三个镜像的 bootstub 完全一致**

### 2.3 tngdisp.ko 显示驱动

| 来源 | SHA256 |
|------|--------|
| Stock ramdisk | `ba00de1b21b32e3af9009e06c9f34dea235954e00bc012aee5b2987f2aaf55ba` |
| TWRP ramdisk | `ba00de1b21b32e3af9009e06c9f34dea235954e00bc012aee5b2987f2aaf55ba` |

**结论：tngdisp.ko 完全一致**

### 2.4 Ramdisk

| 来源 | 大小 | SHA256 |
|------|------|--------|
| stock_cnb19 | 4,421,280 bytes | `125c9db843e0c7fc...` |
| stock_fmb19 | 4,421,296 bytes | `dd4b0d00978bf7d7...` |

> TWRP ramdisk 为完全替换，不与 stock ramdisk 对比。

## 3. Stock Ramdisk 内容分析 (fmb19)

### 3.1 顶层结构

```
├── charger -> /sbin/healthd
├── config_init.sh
├── default.prop
├── fstab / fstab.mofd_v1 / fstab.charger.mofd_v1 / fstab.zram
├── init (634 KB, Android 5.1 init)
├── init.rc / init.aosp.rc / init.mofd_v1.rc / init.common.rc
├── init.wifi.rc / init.wifi.vendor.rc
├── init.bt.rc / init.bt.vendor.rc
├── init.camera.rc / init.firmware.rc
├── init.platform.usb.rc / init.platform.gfx.rc
├── init.recovery.mofd_v1.rc
├── init.zygote64_32.rc
├── intel_prop.cfg
├── lib/modules/ (36 个内核模块)
├── sbin/ (adbd, healthd, intel_prop, thermald)
├── sepolicy / selinux_version
└── verity_key (dm-verity 公钥)
```

### 3.2 关键系统属性 (default.prop)

```properties
ro.zygote=zygote64_32           # 64 位 Zygote + 32 位兼容
ro.swconf.forced=V1_no_modem    # WiFi-only，无 modem
ro.dalvik.vm.native.bridge=libhoudini.so  # Intel ARM 翻译层
ro.opengles.version=196608      # OpenGL ES 3.0
persist.dual_sim=none           # 无 SIM 卡
ro.secure=1 / ro.debuggable=0  # 生产版固件
ro.adb.secure=1                 # 安全 ADB
```

### 3.3 内核模块清单 (36 个 .ko 文件)

#### 显示 / 图形 (4)
| 模块 | 大小 | 说明 |
|------|------|------|
| tngdisp.ko | 2.4 MB | 显示/GPU 驱动 (PowerVR RGX) |
| hdmi_audio.ko | 95 KB | HDMI 音频 |
| fps_throttle.ko | 10 KB | FPS 限制 |
| dfrgx.ko | 30 KB | GPU 动态频率调节 |

#### 音频 (11)
| 模块 | 大小 | 说明 |
|------|------|------|
| arizona-i2c.ko | 7 KB | Arizona SoC I2C |
| snd-soc-arizona.ko | 50 KB | Arizona ASoC |
| snd-soc-wm-adsp.ko | 57 KB | WM DSP 支持 |
| snd-soc-wm-hubs.ko | 92 KB | WM 音频集线器 |
| snd-soc-florida.ko | **3.6 MB** | Florida/WM8958 codec (最大模块) |
| snd-soc-wm8994.ko | 209 KB | WM8994/WM8958 ASoC |
| snd-soc-sst-platform.ko | 644 KB | Intel SST 平台 |
| snd-intel-sst.ko | 320 KB | Intel SST 核心 |
| snd-moor-dpcm-florida.ko | 35 KB | Moorefield DPCM Florida |
| snd-merr-dpcm-wm8958.ko | 45 KB | Merrifield DPCM WM8958 |
| snd-merr-saltbay-wm8958.ko | 37 KB | SaltBay WM8958 |

#### 摄像头 (9)
| 模块 | 大小 | 说明 |
|------|------|------|
| atomisp-css2400b0_v21.ko | 1.1 MB | AtomISP CSS 2400 |
| atomisp-css2401a0_legacy_v21.ko | 1.1 MB | AtomISP CSS 2401 (Legacy) |
| atomisp-css2401a0_v21.ko | 1.1 MB | AtomISP CSS 2401 |
| ov5693.ko | 31 KB | **OV5693 前摄驱动** |
| ov680.ko | 38 KB | OV680 辅助摄像头 |
| ov885x.ko | 58 KB | **OV8858 后摄驱动** |
| lm3559.ko | 21 KB | LM3559 LED 闪光灯 |
| m10mo_isp.ko | 90 KB | M10MO ISP |
| m10mo_spi.ko | 10 KB | M10MO SPI 接口 |

#### WiFi / Bluetooth (3)
| 模块 | 大小 | 说明 |
|------|------|------|
| cfg80211.ko | 748 KB | 无线配置框架 |
| bcmdhd.ko | 1.1 MB | BCM43241 WiFi 驱动 |
| bcm_bt_lpm.ko | 19 KB | BCM 蓝牙低功耗管理 |

#### 其他 (9)
| 模块 | 大小 | 说明 |
|------|------|------|
| mac80211.ko | 799 KB | IEEE 802.11 MAC 层 |
| sep3_15.ko | 190 KB | SEP3 安全引擎 |
| pax.ko | 13 KB | PAX 安全模块 |
| s5k6b2yx.ko | 31 KB | Samsung S5K6B2 (ZenFone 2 遗留) |
| SOCWATCH1_5.ko | 163 KB | Intel SoC Watch 性能工具 |
| test_nx.ko | 6 KB | NX 测试模块 |
| videobuf-core.ko | 34 KB | 视频缓冲核心 |
| videobuf-vmalloc.ko | 12 KB | 视频缓冲 vmalloc |
| libmsrlisthelper.ko | 6 KB | AtomISP MSR 辅助 |

> **注意**: s5k6b2yx.ko (Samsung 传感器) 是 ZenFone 2 遗留模块

### 3.4 TWRP vs Stock 模块对比

| | Stock | TWRP |
|---|---|---|
| 模块总数 | 36 | 1 |
| 保留模块 | — | tngdisp.ko |
| 移除模块 | — | 音频(11), 摄像头(9), WiFi(3), 其他(12) |

TWRP 只需要显示驱动 (tngdisp.ko)，触摸由内核内建的 Goodix GT9xx 驱动处理。

### 3.5 init.mofd_v1.rc 关键硬件初始化

```bash
# 显示驱动 (on init)
insmod /lib/modules/tngdisp.ko

# SilentLake 安全 (on init)
insmod /lib/modules/vidt_driver.ko     # 注意: 不在模块列表中!

# 音频 WM8958 栈 (on boot)
insmod /lib/modules/arizona-i2c.ko
insmod /lib/modules/snd-soc-arizona.ko
insmod /lib/modules/snd-soc-wm-adsp.ko
insmod /lib/modules/snd-soc-wm-hubs.ko
insmod /lib/modules/snd-soc-florida.ko
insmod /lib/modules/snd-soc-wm8994.ko
insmod /lib/modules/snd-soc-sst-platform.ko dpcm_enable=1 dfw_enable=1
insmod /lib/modules/snd-intel-sst.ko
insmod /lib/modules/snd-moor-dpcm-florida.ko
insmod /lib/modules/snd-merr-dpcm-wm8958.ko

# WiFi BCM43241 (on boot)
insmod /lib/modules/cfg80211.ko
insmod /lib/modules/bcmdhd.ko

# 触摸 — ZenFone 2 遗留! (on post-fs)
insmod /lib/modules/rmi4.ko    # ← 文件不存在，静默失败！
```

**重要发现**：
- `insmod /lib/modules/rmi4.ko` — 引用的 rmi4.ko 文件 **不存在** 于 ramdisk 中
- `insmod /lib/modules/vidt_driver.ko` — 同样不存在
- Goodix GT9xx 触摸驱动 **没有** 对应 .ko 文件 → 编译进内核 (built-in)

### 3.6 传感器 I2C 路径 (from init.mofd_v1.rc)

init 脚本中引用的传感器 sysfs 路径：

| 路径 | 设备 |
|------|------|
| `/sys/bus/i2c/devices/5-0019/lis3dh/` | LSM303DLHC 加速度计 |
| `/sys/bus/i2c/devices/5-005c/` | LPS331AP 气压计 |
| `/sys/bus/i2c/devices/5-001e/lsm303cmp/` | LSM303DLHC 磁力计 |
| `/sys/bus/i2c/devices/5-0068/` | L3GD20H 陀螺仪 |

> **注**: init 引用 bus 5，而 SFI DEVS 表列为 bus 8。推测 PSH 传感器集线器在 bus 5 上提供传感器访问。

### 3.7 USB ADB 配置

```
VID: 8087 (Intel)
PID: 0a5d
Product: "Android-Phone"
```

### 3.8 分区挂载表 (fstab.mofd_v1)

| 分区 | 挂载点 | 文件系统 | 选项 |
|------|--------|---------|------|
| system | /system | ext4 | ro,noatime,**verify** (dm-verity) |
| cache | /cache | ext4 | nosuid,nodev,noatime |
| config | /config | ext4 | nosuid,nodev,noatime |
| data | /data | ext4 | nosuid,nodev,noatime,discard,**encryptable** |
| logs | /logs | ext4 | nosuid,nodev |
| persistent | /persistent | emmc | — |
| factory | /factory | ext4 | nosuid,nodev,noatime |

> system 分区启用 dm-verity (有 `verity_key`)，data 分区支持加密。

## 4. CNB19 vs FMB19 差异

| 对比项 | CNB19 | FMB19 |
|--------|-------|-------|
| Kernel | 不同 (b1b77448...) | 不同 (d8be6cfb...) |
| Ramdisk | 不同 (4,421,280 bytes) | 不同 (4,421,296 bytes) |
| Second | **相同** | **相同** |
| Cmdline | 相同 | 相同 |

两版使用不同的内核构建（可能有 GMS/非 GMS 差异），但 bootstub 完全一致。

## 5. 结论

### 5.1 TWRP 来源确认

**TWRP boot 镜像使用的是 Nokia N1 fmb19 (国际版) 的原始内核，非 ZenFone 2 修改版。**

证据：
1. TWRP kernel 与 stock_boot_fmb19 kernel SHA256 完全一致
2. Bootstub (second) 三个镜像完全一致
3. tngdisp.ko 完全一致
4. init.recovery.mofd_v1.rc 内容一致
5. cmdline 使用相同的 SPID/serialno 占位符格式

### 5.2 架构总结

```
┌─────────────────────────────────────────────┐
│             IFWI 固件 (SFI 表)              │
│  · 基于 ZenFone 2 (Moorefield) 固件         │
│  · 包含遗留设备条目 (imx135/r69001)         │
│  · Nokia N1 设备在末尾追加 (ov8858/ov5693)  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│            Kernel (Nokia N1 专用)            │
│  · Goodix GT9xx 触摸驱动 (built-in)         │
│  · 版本 3.10.62-x86_64_moor                 │
│  · cnb19/fmb19 两个不同编译版本             │
└─────────────────────────────────────────────┘
                    ↓
┌──────────────────────┬──────────────────────┐
│   Stock Ramdisk      │    TWRP Ramdisk      │
│  · Android 5.1       │   · TWRP Recovery    │
│  · 36 个 .ko 模块    │   · 仅 tngdisp.ko   │
│  · dm-verity         │   · BusyBox 工具集   │
│  · 完整音频/相机栈   │   · 分区管理/备份    │
└──────────────────────┴──────────────────────┘
```

### 5.3 ZenFone 2 遗留痕迹

仅存在于固件层和初始化脚本中，**不影响实际运行**：

| 遗留项 | 位置 | 影响 |
|--------|------|------|
| imx135/imx132 SFI 条目 | IFWI DEVS 表 | 无（驱动未匹配到设备） |
| r69001-ts-i2c SFI 条目 | IFWI DEVS 表 | 无（总线/地址不匹配） |
| `insmod rmi4.ko` | init.mofd_v1.rc | 无（文件不存在，静默失败） |
| s5k6b2yx.ko | lib/modules/ | 无（Samsung 传感器驱动，设备上无此硬件） |

---

*分析基于: stock_boot_cnb19.img, stock_boot_fmb19.img, twrp_dev/kernel, twrp_dev/second*
