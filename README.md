# Nokia N1 Boot 开发项目

## 项目概述

Nokia N1 (Intel Moorefield Z3580) 平板的自定义启动镜像开发项目。
基于原厂 3.10.62 内核 + 修改的 TWRP ramdisk + Intel Bootstub 构建 boot.img。

**当前状态**: TWRP Recovery 正常运行，boot 分区已扩至 32MB，内置 strace/devmem2/pmic_tool 等调试工具。

## 目录结构

```
n1_dev/
├── README.md                       # 本文件
├── docs/
│   ├── HARDWARE_REFERENCE.md       # 硬件参数完整参考 (已验证)
│   ├── BOOT_FLOW_PARTITION_ANALYSIS.md  # 启动流程与 GPT 分区分析
│   ├── BOOTLOADER_DISASSEMBLY_ANALYSIS.md  # Bootstub 反汇编分析
│   ├── REVERSE_ENGINEERING.md      # 原厂 boot.img 逆向分析 (历史)
│   ├── PACKAGING.md                # Boot image 打包指南
│   └── VERIFICATION_CHECKLIST.md   # Ground Truth 验证清单
├── droidboot_analysis/
│   └── report.md                   # Droidboot 镜像系统分析
├── ifwi_analysis/
│   ├── report.md                   # IFWI 固件深度分析
│   └── unlock_warning_mechanism.md # AVB / 解锁机制
├── twrp_dev/
│   ├── boot_final.img              # 当前使用的 boot 镜像 (15.2MB)
│   ├── kernel                      # 原厂 3.10.62 bzImage (64-bit)
│   ├── second                      # 原厂 Bootstub 1.4 (8KB)
│   ├── ramdisk                     # 原始 TWRP ramdisk (gzip)
│   ├── BOOT_CHAIN_GUIDE.md         # 启动链与 kexec 实现指南
│   ├── DEVELOPMENT_GUIDE.md        # TWRP ramdisk 修改指南
│   ├── ramdisk_unpacked/           # 解压的 ramdisk (含自定义工具)
│   ├── tools_src/                  # 自定义工具源码
│   │   ├── pmic_tool.c             # PMIC 寄存器读写工具
│   │   └── n1_display.c            # 显示/背光/GPIO 控制工具
│   └── device_tree/nokia/Nokia_N1/ # TWRP 设备树
│       └── BUILD_GUIDE.md          # 设备树构建指南
├── backups/
│   └── gpt/                        # GPT 分区表备份 + 修改脚本
│       ├── gpt_primary.bin         # 修改前的主 GPT
│       ├── gpt_secondary.bin       # 修改前的备份 GPT
│       └── modify_gpt.py          # GPT 修改 Python 脚本
└── images/original/                # 原厂镜像备份
    ├── droidboot.img
    ├── stock_boot_*.img
    └── ifwi_boot1.bin
```

## 关键技术参数

| 参数 | 值 |
|------|-----|
| SoC | Intel Atom Z3580 (Moorefield, Silvermont) |
| 内核 | 3.10.62-x86_64_moor (64-bit) |
| 用户空间 | 32-bit TWRP (i386) |
| Boot 分区 | **32 MB** (原 16MB, 已扩展) |
| Recovery 分区 | 已删除 (合并入 boot) |
| Boot.img 大小 | ~15.2 MB (余 16.8 MB) |
| 显示面板 | JDI 1536×2048, MIPI DSI |

## 内置调试工具

boot.img 中的 ramdisk 包含以下工具 (musl-gcc x86_64 静态链接):

| 工具 | 大小 | 用途 |
|------|------|------|
| strace 6.12 | 1.4 MB | 系统调用跟踪 |
| devmem2 | 34 KB | 物理内存/MMIO 读写 |
| pmic_tool | 38 KB | PMIC (Shadycove) 寄存器读写 |
| n1_display | 34 KB | 显示/背光/GPIO 控制 |
| n1_hwinfo.sh | 5.7 KB | 硬件信息采集脚本 |

> 工具编译要求: `musl-gcc -static` (64-bit)。glibc 静态链接不兼容 (要求 kernel ≥ 4.4.0，设备为 3.10.62)。

## 关键里程碑

- [x] 原厂 boot.img 逆向分析完成
- [x] IFWI/Bootstub/Droidboot 启动链理解完毕
- [x] TWRP Recovery 正常启动
- [x] ADB over USB 工作
- [x] Kexec 功能验证 (64-bit patched kexec)
- [x] GPT 分区表修改: boot 16MB → 32MB
- [x] 自定义调试工具编译并内置
- [ ] 启动日志分析与异常修复
- [ ] 自定义内核启动 (via kexec)
