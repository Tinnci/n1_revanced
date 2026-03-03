# Nokia N1 `droidboot.img` 深度分析报告

## 1. 文件结构概述

`droidboot.img` 是一个标准的 **Android Boot Image (ANDROID!)** 格式，大小为 **17MB**，包含以下组成部分：

| 组件 | 大小 | 加载地址 | 描述 |
|------|------|---------|------|
| **kernel** | 6.3MB | `0x10008000` | Linux 内核 bzImage |
| **ramdisk** | 7.8MB | `0x11000000` | gzip 压缩的 cpio 文件系统 |
| **second** | 8KB | `0x10f00000` | Intel **Bootstub** (二级引导) |

---

## 2. 关键组件分析

### A. 内核 (kernel) — **完整 64 位 x86-64 模式**

```
Linux kernel x86 boot executable, bzImage
version 3.10.62-x86_64_moor (sam@topaz4)
#1 SMP PREEMPT Wed Nov 25 10:05:47 CST 2015
protocol 2.12, legacy 64-bit entry point
```

**结论：CPU 在运行时使用 完整 64 位 x86-64 模式**，内核明确编译为 `x86_64`。

### B. Ramdisk — 完整的 Droidboot 环境

所有的用户空间二进制均为 **64 位静态链接 ELF**：

```
init:        ELF 64-bit LSB executable, x86-64, statically linked
adbd:        ELF 64-bit LSB executable, x86-64, statically linked
healthd:     ELF 64-bit LSB executable, x86-64, statically linked
intel_prop:  ELF 64-bit LSB executable, x86-64, statically linked
thermald:    ELF 64-bit LSB executable, x86-64, statically linked
```

功能包括：
- **adbd** — ADB 守护进程，用于 fastboot/adb 连接
- **healthd** — 电池/充电管理（`charger` 链接到此）
- **thermald** — 热管理守护进程
- **init** — Android init 系统（ueventd/watchdogd 是其符号链接）

**显示驱动**：加载 `tngdisp.ko` 内核模块（Intel Tangier 显示驱动）

### C. Second Bootloader — Intel **Bootstub 1.4**

这是一个 **32 位保护模式** 的初始化代码：

```
Bootstub Version: 1.4 ...
Using bzImage to boot
Jump to kernel 32bit entry
PNW detected / CLV detected / MRD detected / ANN detected
```

**工作流程**：
1. **16 位实模式** → 初始化设置
2. **32 位保护模式** → Bootstub 运行，设置 E820 内存映射
3. **跳转到内核 32 位入口点** → Linux 内核再自行切换到 **64 位长模式**

---

## 3. 引导链分析

```
┌───────────────────────────────────────────────────────────────────────┐
│                     Intel Moorefield Boot Chain                       │
└───────────────────────────────────────────────────────────────────────┘

  ┌─────────┐    ┌─────────────┐    ┌───────────────┐    ┌─────────────┐
  │  IFWI   │───▶│  DnX/SCU    │───▶│   Bootstub    │───▶│  Linux      │
  │ (ROM)   │    │  Firmware   │    │   (second)    │    │  Kernel     │
  │ 16-bit  │    │  32-bit PM  │    │  32-bit PM    │    │  64-bit LM  │
  └─────────┘    └─────────────┘    └───────────────┘    └─────────────┘
                                           │
                                           ▼
                               ┌─────────────────────────┐
                               │  Android Ramdisk (init) │
                               │     64-bit userspace    │
                               └─────────────────────────┘
```

**关于 boot.img 的引导**：
Nokia N1 的引导加载程序通过 Bootstub (second stage) 来引导 Linux 内核。Bootstub 负责解析 Bootloader 传递的信息并跳转到内核的 32 位入口。

---

## 4. CPU 模式总结

| 引导阶段 | CPU 模式 | 备注 |
|---------|---------|------|
| **IFWI/ROM 启动** | 16 位实模式 | x86 标准启动方式 |
| **Bootstub (second)** | 32 位保护模式 | 设置 GDT、A20、E820 |
| **Linux 内核入口** | 32 位保护模式 | bzImage setup code |
| **Linux 内核运行** | **64 位长模式** | x86_64 完整模式 |
| **Userspace (init, adbd...)** | **64 位长模式** | ELF64 静态链接 |

**最终答案**：
- **Droidboot 运行时** = **完整 64 位 x86-64 长模式**
- **不是 16 位**（只在最初 ROM 解析时短暂使用）
- **不是 32 位**（只在 Bootstub 和内核解压阶段短暂使用）
