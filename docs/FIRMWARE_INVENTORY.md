# Nokia N1 固件文件清单

## 概述

从 Nokia N1 的 `/system/etc/firmware/` 分区提取的全部 36 个固件二进制文件。
这些文件在内核编译和运行时被各驱动通过 `request_firmware()` API 加载。

**提取日期**: 2025-03-03
**来源**: Nokia N1 A5CNB19 系统分区 (`/dev/block/mmcblk0p17`)
**总大小**: ~33 MB (36 文件)

---

## 1. 固件分类

### 1.1 GPU/视频处理 (tngdisp.ko 使用)

| 文件 | 大小 | 功能 | 加载方式 |
|------|------|------|---------|
| `topaz.bin.prod` | 847 KB | TOPAZ HP 视频**编码**器固件 (正式版) | `tng_securefw(dev, "topaz", "VEC", ...)` |
| `topaz.bin.verf` | 847 KB | TOPAZ HP 视频编码器固件 (验证版) | 优先尝试 .verf，失败回退 .prod |
| `msvdx.bin.prod` | 58 KB | MSVDX 视频**解码**器固件 (正式版) | `tng_securefw(dev, "msvdx", "VED", ...)` |
| `msvdx.bin.verf` | 58 KB | MSVDX 视频解码器固件 (验证版) | 优先尝试 .verf，失败回退 .prod |
| `vsp.bin.prod` | 1.9 MB | VSP 视频信号处理器固件 (正式版) | `tng_securefw(dev, "vsp", "VSP", ...)` |
| `vsp.bin.verf` | 1.9 MB | VSP 视频信号处理器固件 (验证版) | 同上 |

**说明**: `.verf` = verification/debug 版本, `.prod` = production 版本。`tng_securefw()` 函数先尝试加载 `.verf`，失败后回退到 `.prod`。固件通过 sep54 安全引擎验证后加载到专用 IMR (Isolated Memory Region) 中。

### 1.2 摄像头 ISP (AtomISP 使用)

| 文件 | 大小 | 功能 |
|------|------|------|
| `shisp_2401a0_v21.bin` | 12 MB | ISP 2401 主固件 (A0 stepping, CSS v2.1) |
| `shisp_2401a0_legacy_v21.bin` | 11 MB | ISP 2401 Legacy 版固件 |
| `isp_acc_chromaproc_css21_2401.bin` | 85 KB | ISP 加速器: 色度处理 |
| `isp_acc_deghosting_v2_css21_2401.bin` | 101 KB | ISP 加速器: 去重影 |
| `isp_acc_lumaproc_css21_2401.bin` | 213 KB | ISP 加速器: 亮度处理 |
| `isp_acc_mfnr_em_css21_2401.bin` | 291 KB | ISP 加速器: 多帧降噪 (MFNR) |
| `isp_acc_multires_v2_css21_2401.bin` | 185 KB | ISP 加速器: 多分辨率处理 |
| `isp_acc_sce_em_css21_2401.bin` | 273 KB | ISP 加速器: 场景评估 |
| `isp_acc_warping_v2_css21_2401.bin` | 220 KB | ISP 加速器: 几何畸变校正 |
| `isp_acc_warping_v2_em_css21_2401.bin` | 225 KB | ISP 加速器: 几何畸变校正 (扩展) |
| `ov680_fw.bin` | 12 KB | OV680 3D 深度感知传感器固件 |

**说明**: Nokia N1 使用 ISP 2401 (AtomISP)。固件按硬件版本自动选择: ISP2401 → `shisp_2401a0_v21.bin`, ISP2401_LEGACY → `shisp_2401a0_legacy_v21.bin`。

### 1.3 WiFi/蓝牙 (BCM43xx)

| 文件 | 大小 | 功能 |
|------|------|------|
| `fw_bcmdhd.bin_43241_b4` | 392 KB | **BCM43241** WiFi 固件 (STA 模式) |
| `fw_bcmdhd_apsta.bin_43241_b4` | 392 KB | BCM43241 WiFi 固件 (AP+STA 模式) |
| `bcmdhd_aob.cal_43241_b4` | 2.5 KB | BCM43241 WiFi 校准数据 |
| `BCM43241b4_003.001.009.0061.0293.hcd` | 15 KB | **BCM43241** 蓝牙固件 (HCD patchram) |
| `fw_bcmdhd.bin_4354_a1` | 577 KB | BCM4354 WiFi 固件 (STA) — *不使用* |
| `fw_bcmdhd_apsta.bin_4354_a1` | 577 KB | BCM4354 WiFi 固件 (AP+STA) — *不使用* |
| `bcmdhd_aob.cal_4354_a1` | 3.7 KB | BCM4354 校准数据 — *不使用* |
| `BCM4350C0_003.001.009.0061.0293.hcd` | 65 KB | BCM4350 蓝牙固件 — *不使用* |

**说明**: Nokia N1 SFI 表注册 `bcm43xx_clk_vmmc` (SDIO bus 1)。根据固件文件命名和设备变体，**Nokia N1 使用 BCM43241 B4** WiFi/BT 芯片。BCM4354 文件是 ZenFone 2 变体的，Nokia N1 不使用。

Nokia N1 defconfig 中需要设置:
```
CONFIG_BCMDHD_FW_PATH="/system/etc/firmware/fw_bcmdhd.bin_43241_b4"
```

### 1.4 音频 SST (Intel Smart Sound Technology)

| 文件 | 大小 | 功能 |
|------|------|------|
| `fw_sst_1495.bin` | 559 KB | SST DSP 主固件 (PCI ID 0x1495 = Moorefield) |
| `dfw_sst.bin` | 149 KB | SST DFW (Digital Firmware Widgets) 配置固件 |

**说明**: `fw_sst_1495.bin` 通过 `request_firmware_nowait()` 异步加载。文件名由 `fw_sst_` + PCI 设备 ID (`0x1495`) + `.bin` 组成。`dfw_sst.bin` 被内嵌到内核中 (CONFIG_EXTRA_FIRMWARE="dfw_sst.bin")。

### 1.5 传感器集线器 (PSH)

| 文件 | 大小 | 功能 |
|------|------|------|
| `psh.bin` | 169 KB | PSH 默认固件 (fallback) |
| `psh.bin.0000.0000.0008.0008.0000.0002` | 172 KB | SPID 特定固件变体 |
| `psh.bin.0000.0000.0008.0008.0001.0000` | 167 KB | SPID 特定固件变体 |
| `psh.bin.0000.0000.0008.0008.0001.0001` | 167 KB | SPID 特定固件变体 |
| `psh.bin.0000.0000.0008.0008.0002.0001` | 172 KB | SPID 特定固件变体 |

**说明**: PSH 驱动根据 SPID 选择固件:
1. 先尝试 `psh.bin.{customer_id}.{vendor_id}.{mfg_id}.{platform_id}.{product_id}.{hw_id}`
2. 失败则回退到 `psh.bin`

Nokia N1 的 Bootstub 修正后 SPID 为 `0000:0000:0000:0008:0000:0000`，对应的文件名应为 `psh.bin.0000.0000.0000.0008.0000.0000`，但该文件不存在，所以会回退到 `psh.bin`。

### 1.6 触摸屏固件 (Synaptics — ZenFone 2 专用)

| 文件 | 大小 | 功能 |
|------|------|------|
| `s3202_gff.img` | 47 KB | Synaptics S3202 GFF 触摸固件 |
| `s3202_ogs.img` | 47 KB | Synaptics S3202 OGS 触摸固件 |
| `s3400_cgs.img` | 62 KB | Synaptics S3400 CGS 触摸固件 |
| `s3400_igzo.img` | 62 KB | Synaptics S3400 IGZO 触摸固件 |

**⚠️ Nokia N1 不使用这些文件** — Nokia N1 使用 Goodix GT928 触摸控制器，触摸固件内嵌在 GT9xx 驱动源码中 (`gt9xx_firmware.h`)。

---

## 2. Nokia N1 实际使用的固件文件

以下是 Nokia N1 **实际加载**的固件文件（按启动顺序）:

| 序号 | 文件 | 加载者 | 阶段 |
|------|------|--------|------|
| 1 | *(内嵌)* `intel_mid_remoteproc.fw` | remoteproc | 内核启动 0.186s |
| 2 | `dfw_sst.bin` | SST 平台驱动 | *(内嵌在内核中)* |
| 3 | `fw_sst_1495.bin` | SST DSP 驱动 | 音频子系统初始化 |
| 4 | `psh.bin` | PSH 驱动 | 传感器初始化 |
| 5 | `fw_bcmdhd.bin_43241_b4` | bcmdhd 模块 | WiFi 启用时 |
| 6 | `BCM43241b4_003.001.009.0061.0293.hcd` | BT 驱动 | 蓝牙启用时 |
| 7 | `shisp_2401a0_v21.bin` | AtomISP | 相机打开时 |
| 8 | `isp_acc_*.bin` (7个) | AtomISP ACC | 摄影功能使用时 |
| 9 | `topaz.bin.prod` | tngdisp | 视频编码时 |
| 10 | `msvdx.bin.prod` | tngdisp | 视频解码时 |
| 11 | `vsp.bin.prod` | tngdisp | 视频处理时 |

---

## 3. 内核编译所需的固件文件

### 3.1 必须内嵌到内核中的固件

```
CONFIG_EXTRA_FIRMWARE="dfw_sst.bin"
CONFIG_EXTRA_FIRMWARE_DIR="firmware"
```

此外，`intel_mid_remoteproc.fw` 通过 `.builtin_fw` 段内嵌——源码中应已包含此文件。

### 3.2 运行时需要放在 `/system/etc/firmware/` 的固件

```
# 最小功能集 (显示+音频+WiFi+传感器):
psh.bin
fw_sst_1495.bin
fw_bcmdhd.bin_43241_b4
BCM43241b4_003.001.009.0061.0293.hcd

# 视频播放/编码:
topaz.bin.prod
msvdx.bin.prod
vsp.bin.prod

# 相机:
shisp_2401a0_v21.bin (或 shisp_2401a0_legacy_v21.bin)
isp_acc_chromaproc_css21_2401.bin
isp_acc_deghosting_v2_css21_2401.bin
isp_acc_lumaproc_css21_2401.bin
isp_acc_mfnr_em_css21_2401.bin
isp_acc_multires_v2_css21_2401.bin
isp_acc_sce_em_css21_2401.bin
isp_acc_warping_v2_css21_2401.bin
isp_acc_warping_v2_em_css21_2401.bin
ov680_fw.bin
```

---

## 4. 本地文件位置

固件文件已提取到: `/tmp/n1_firmware/firmware/`
SHA256 校验和: `/tmp/n1_firmware_sha256.txt`

> **⚠️ 版权声明**: 这些固件文件为 Nokia/Intel/Broadcom 等公司的闭源专有二进制，不应上传到公开 Git 仓库。仅用于本地内核开发。
