# Nokia N1 Boot Images

## 当前使用

当前设备上运行的 boot 镜像为 `twrp_dev/boot_final.img`，直接通过 `dd` 写入 boot 分区。

## 目录: original/

原始固件镜像备份（只读参考，不要修改）：

- `droidboot.img` - 原厂 Droidboot/fastboot 镜像
- `stock_boot_*.img` - 原厂 boot 分区备份
- `ifwi_boot1.bin` - IFWI Boot1 固件提取

## Boot 镜像格式

Nokia N1 使用标准 Android mkbootimg 格式（含 second stage）：

```
+------------------+
| boot header      |  1 page (2048 bytes)
+------------------+
| kernel (bzImage)  |  ~6.3 MB
+------------------+
| ramdisk (gzip)   |  ~9 MB
+------------------+
| second (Bootstub) |  ~8 KB
+------------------+
```

### mkbootimg 参数

```bash
mkbootimg \
  --kernel kernel --ramdisk ramdisk.gz --second second \
  --base 0x10000000 --kernel_offset 0x00008000 \
  --ramdisk_offset 0x01000000 --second_offset 0x00f00000 \
  --tags_offset 0x00000100 --pagesize 2048 \
  --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M \
    androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 gpt bootboost=1" \
  --output boot.img
```

> **重要**: 打包 ramdisk 时必须使用 `fakeroot` 确保文件属主为 root:root，否则设备会卡在 splash screen。
> 详见 `twrp_dev/DEVELOPMENT_GUIDE.md`。
