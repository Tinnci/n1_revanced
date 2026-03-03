# TWRP 基线开发指南

## 目录结构

```
/home/drie/n1_dev/twrp_dev/
├── kernel           # 原始内核 (64-bit bzImage)
├── ramdisk          # 原始 ramdisk (gzip 压缩)
├── second           # Bootstub
└── ramdisk_unpacked/  # 解压后的 ramdisk (可修改)
    ├── init           # 32-bit 静态链接 init
    ├── init.rc        # 主配置文件
    ├── sbin/
    │   ├── busybox    # 32-bit (无 devmem)
    │   ├── recovery   # TWRP 主程序
    │   ├── adbd       # ADB 守护进程
    │   └── ...
    ├── lib/modules/
    │   └── tngdisp.ko # 显示驱动
    └── twres/         # TWRP 资源文件
```

## TWRP 特点

1. **32-bit 用户空间** - init 和所有工具都是 i386
2. **完整的 busybox** - 但没有 devmem
3. **SELinux 宽松模式** - `set_permissive` 服务
4. **USB 配置完整** - mtp,adb 功能
5. **持续运行** - 不会像 droidboot 一样自动退出

## 如何修改

### 方法 1: 添加启动脚本 (推荐)

在 `init.rc` 的 `on boot` 部分添加自定义脚本:

```rc
on boot
    ifup lo
    hostname localhost
    domainname localdomain
    # 添加自定义初始化
    exec /sbin/custom_init.sh
    class_start default
```

### 方法 2: 替换/添加工具

如需 devmem 功能，需要:
1. 编译 32-bit 静态版本的 devmem 工具
2. 或使用已有的 32-bit busybox (带 devmem)

## 重新打包命令

```bash
# 1. 重新打包 ramdisk
cd /home/drie/n1_dev/twrp_dev/ramdisk_unpacked
find . | cpio -o -H newc | gzip > ../ramdisk_new.gz

# 2. 创建新的 boot.img
mkbootimg \
    --kernel ../kernel \
    --ramdisk ../ramdisk_new.gz \
    --second ../second \
    --base 0x10000000 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x01000000 \
    --second_offset 0x00f00000 \
    --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx androidboot.serialno=01234567890123456789 gpt bootboost=1" \
    --pagesize 2048 \
    -o ../twrp_modified.img

# 3. 刷入测试
fastboot flash boot /home/drie/n1_dev/twrp_dev/twrp_modified.img
```

## 注意事项

1. **工具必须是 32-bit i386** - 内核虽然是 64-bit，但用户空间是 32-bit
2. **kexec 必须是 64-bit** - 参见 `LK2ND_IMPLEMENTATION.md` 中的 kexec 编译指南
3. **保持 SELinux 宽松** - 否则自定义脚本可能被阻止
4. **USB 配置已完善** - TWRP 已经正确配置了 USB gadget
5. **kexec 前必须挂载 debugfs** - `mount -t debugfs none /sys/kernel/debug`

## kexec 快速参考

```bash
# 推送 64-bit kexec 到设备
adb push twrp_dev/bin/kexec_x86_64_static /tmp/kexec64
adb shell chmod +x /tmp/kexec64

# 关键: 挂载 debugfs (否则 hardware_subarch 错误)
adb shell mount -t debugfs none /sys/kernel/debug

# 加载并执行
adb shell /tmp/kexec64 -l /tmp/kernel --initrd=/tmp/ramdisk --reuse-cmdline -i
adb shell /tmp/kexec64 -e

# 判断 kexec 是否成功: 检查 e820 首条 entry
# kexec boot: 从 0x400 开始; 正常 boot: 从 0x0 开始
adb shell 'dmesg | grep "BIOS-e820" | head -1'
```

详细指南见 `LK2ND_IMPLEMENTATION.md`。
