# Nokia N1 固件和二进制文件备份

## SFI 表 (`sfi_tables/`)

从 `/sys/firmware/sfi/tables/` 通过 `adb pull` 提取的原始 SFI 固件表。

| 文件 | 大小 | 条目数 | 说明 |
|------|------|--------|------|
| sfi_devs.bin | 1,174 bytes | 46 | SFI 设备表 (IPC/I2C/SPI/SD/HSI/UART/USB) |
| sfi_gpio.bin | 2,404 bytes | 70 | SFI GPIO 映射表 |
| sfi_oem0.bin | 120 bytes | — | OEM0 热配置表 |
| sfi_oemb.bin | 96 bytes | — | OEMB 板级信息表 (含序列号) |

解析格式详见 [HARDWARE_PARAMETERS.md](../docs/HARDWARE_PARAMETERS.md)。

## Stock 内核模块 (`stock_modules/`)

从 `stock_boot_fmb19.img` ramdisk 的 `/lib/modules/` 提取的原始内核模块。

共 36 个 `.ko` 文件，总大小约 14 MB。

所有模块签名密钥: `Magrathea: Glacier signing key` (SHA256, CONFIG_MODULE_SIG_FORCE=y)

内核版本: `3.10.62-x86_64_moor`

详细清单详见 [STOCK_BOOT_ANALYSIS.md](../docs/STOCK_BOOT_ANALYSIS.md#33-内核模块清单-36-个-ko-文件)。
