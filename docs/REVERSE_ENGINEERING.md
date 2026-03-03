# Nokia N1 原厂 Boot Image 逆向分析

> **⚠️ 可信度说明**: 本文档包含两类内容：
> - ✅ **事实**: 基于工具输出 (file/hexdump/strings) 的确认数据
> - ⚠️ **推断**: 基于数据的逻辑推理，可能不完全准确
>
> “三大根本问题”部分是对比旧 Zig 方案 vs 原厂的分析，地址问题已在当前 TWRP 方案中修复。

## 关键发现

### 原厂 Boot 结构 vs 我们的实现

```
【原厂 stock_boot_cnb19.img】

Boot Image Header @ 0x000
├─ Magic: "ANDROID!"
├─ Kernel size: 0x609990 (6.3 MB)
├─ Kernel load addr: 0x10008000 ❌ 不同!
├─ Ramdisk size: 0x4376a0 (4.4 MB)
├─ Ramdisk load addr: 0x11000000
├─ Second size: 0x2000 (8 KB)
├─ Second load addr: 0x10f00000
└─ Tags addr: 0x10000100

Kernel @ offset 0x800 (Page 1)
├─ 实际内容: bzImage (Linux kernel, 6.1M 大小)
│           ├─ x86 boot header
│           ├─ Setup code
│           └─ Compressed kernel image (gzip)
├─ 特征: "Linux kernel x86 boot executable"
└─ Version: 3.10.62-x86_64_moor

Second bootloader @ offset 0x60a800
├─ 大小: 8 KB
└─ 用途: (needs analysis)

Ramdisk @ offset 0x60a800 + aligned
└─ gzip 压缩的 CPIO 存档


【我们的 boot.img】

Boot Image Header @ 0x000
├─ Magic: "ANDROID!" ✓
├─ Kernel size: 0x62fe4 (397 KB)
├─ Kernel load addr: 0x01100000 ❌ 完全不同!
├─ Ramdisk size: 0x0e (14 字节)
├─ Ramdisk load addr: 0x03000000
├─ Tags addr: 0x01000100
└─ Page size: 2048

Kernel @ offset 0x800
├─ 实际内容: 裸 Zig 二进制
└─ 特征: "data" (无识别的格式)
```

### 🚨 三大根本问题

#### 1. **内存基址完全错误**
```
原厂:  Kernel @ 0x10008000   (Droidboot 期望)
我们:  Kernel @ 0x01100000   (我们错误配置)

差异: 0x10008000 - 0x01100000 = 0x0EF08000 (240 MB!)
```

这意味着：
- Droidboot **可能在内存重新映射后** 把内核加载到 0x10008000
- 我们配置的 0x01100000 被 Droidboot **忽略或冲突**

#### 2. **Second bootloader (8 KB)**
原厂 boot.img 包含一个 8 KB 的 "second bootloader"，但我们：
- 没有包含它
- 不知道它的作用
- 可能需要它来初始化硬件

#### 3. **内核格式完全不同**
```
原厂:  bzImage (Linux kernel boot 可执行文件)
       ├─ x86 实模式 boot stub
       ├─ Protected mode 代码
       └─ gzip 压缩的内核
       
我们:  裸 ELF 二进制 + objcopy 到纯二进制
       └─ 无 boot 协议支持
```

---

## Droidboot 启动流程推断

> **⚠️ 以下是推断，非完全验证的事实**

基于逆向分析，Droidboot 的启动流程可能是：

```
1. Droidboot 读取 boot.img 头部
   └─ 验证 Magic: "ANDROID!"
   
2. 根据 boot.img 设置地址映射
   ├─ Kernel load addr 来自 header (0x10008000)
   ├─ 可能有 IOMMU/MMU 重映射
   └─ 内存布局可能是动态的
   
3. 加载 Second bootloader (可选)
   ├─ 从 boot.img 中提取
   ├─ 加载到 0x10f00000
   └─ 可能初始化硬件 (GPU, USB 等)
   
4. 加载 Kernel
   ├─ 从 boot.img 中提取
   ├─ 加载到 header 指定的地址 (0x10008000)
   └─ 可能解压缩 (bzImage 是 gzip 的)
   
5. 跳转到内核入口
   ├─ 设置寄存器:
   │  ├─ EAX = 0 (Linux boot protocol)
   │  ├─ ESI = ptr to boot params
   │  ├─ EDI = 0
   │  └─ EBP = 0
   ├─ 设置 CR0 进入保护模式
   └─ 调用内核代码
```

---

## 修复策略 (已完成 — 仅作历史记录)

> 以下修复已在当前 TWRP-based lk2nd 方案中实施。旧 Zig 方案已废弃。

### ✅ 选项 A: 正确配置内核加载地址 (已完成)

更改我们的 boot.img 打包参数：

```bash
# 当前 (错误)
BASE_ADDR="0x01000000"
KERNEL_OFFSET="0x00100000"
KERNEL_LOAD = 0x01100000

# 应改为 (正确)
BASE_ADDR="0x10000000"        # Droidboot 的真实基址
KERNEL_OFFSET="0x00008000"    # 真实内核偏移
KERNEL_LOAD = 0x10008000      # 匹配原厂
```

### 选项 B: 分析 Second bootloader

提取并分析 second 文件来了解其作用：

```bash
hexdump -C second | head -50  # 查看内容
strings second               # 查找字符串
objdump -d second           # 反汇编 (如果是代码)
```

### 选项 C: 采用 bzImage 格式

将我们的 Zig bootloader 编译成 bzImage 而非裸二进制：
- 需要 x86 boot header
- 需要 setup 代码部分
- 需要内核 gzip 压缩

---

## 确认地址方案

运行以下命令确认原厂使用的真实物理地址：

```bash
# 解压原厂 kernel
cd /tmp/stock_analysis
magiskboot unpack /home/drie/n1_dev/images/original/stock_boot_cnb19.img

# 查看 Linux kernel 头部信息
objdump -D kernel | head -100

# 或用 strings 查找地址线索
strings kernel | grep -E "0x[0-9a-f]{8}|load|entry|start"
```

---

## 下一步行动

1. ✅ 确认原厂 kernel 的加载地址是真的 0x10008000
2. ✅ 分析 second bootloader 的内容和作用
3. ✅ 修改我们的 boot.img 打包参数
4. ✅ 测试新的地址配置
5. ✅ 如果还是不工作，考虑 bzImage 格式

---

**关键参数对比表**

| 参数 | 原厂 | 我们 | 修复后 |
|------|------|------|--------|
| Kernel Load Addr | 0x10008000 | 0x01100000 | 0x10008000 |
| Base Addr | 0x10000000 | 0x01000000 | 0x10000000 |
| Kernel Offset | 0x8000 | 0x100000 | 0x8000 |
| Ramdisk Addr | 0x11000000 | 0x03000000 | (保持) |
| Tags Addr | 0x10000100 | 0x01000100 | 0x10000100 |

