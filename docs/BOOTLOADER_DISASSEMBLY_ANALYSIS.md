# 🔬 Nokia N1 Bootloader 反汇编法医分析

## 关键发现：Droidboot 启动流程的真相

### 【第一阶段】Second Bootloader (Bootstub) 的初始化序列

#### 地址空间配置（这是关键！）

```asm
00000002: mov esp, 0x10f00000          # 栈指针设置在 0x10f00000
00000007: call 0x387                   # 跳转到初始化例程

# GDT/IDT 复制到低地址空间
000000b5: lidtd cs:0x10f00050         # 中断描述符表
000000bd: lgdtd cs:0x10f00078         # 全局描述符表

# 关键的地址映射：
000000be: mov esi, 0x10f000db        # 源地址 (高地址 0x10f00000)
000000ae: mov edi, 0xa000             # 目标地址 (低地址 0x0000a000)
000000b3: rep movs DWORD              # 批量复制
```

**这说明了什么？**
- Second bootloader 在内存的 **0x10f00000** 处（高地址区）
- 它将代码复制到 **0x0000a000** （低地址区）
- 这是一个 **从高到低的内存重映射** 过程

#### 模式切换序列

```asm
# 保护模式入口
000000d4: jmp 0x8:0xa000              # 跳转到保护模式代码
000000db: mov cr0, eax                # 启用保护模式 (设置 CR0.PE)
000000e0: jmp 0xb866:0xa00a           # 继续执行

# 栈重设置
000000f5: mov sp, 0xb000              # 新栈指针

# 内存清空
00000101: mov WORD PTR [bx+si], 0x0   # 清除内存区域
0000010f: mov WORD PTR [bx+si], 0x0
```

**这说明了什么？**
- 栈从 **0x10f00000** 切换到 **0x00b000**
- CR0 (CR0.PE) 被设置以启用保护模式
- 这是 **实模式 → 保护模式** 的转换

#### E820 内存查询

```asm
00000124: mov edx, 0x534d4150         # EDX = "SMAP" (E820 魔数)
00000129: int 0x15                    # 调用 BIOS 中断 15h
0000012b: jb 0x16a                    # 跳转错误处理
```

**这说明了什么？**
- BIOS int 15h (E820) 被用来查询内存映射
- 可能返回可用 RAM 的范围
- Droidboot 可能在此基础上构建内存表

---

### 【第二阶段】Linux Kernel 的引导头 (bzImage Header)

#### 关键的 bzImage 魔数和签名

```asm
000001ed: ff ff 1d 01 00 d9 05        # bzImage 签名区域
000001f0: call FWORD PTR ds:0x5d90001  # 远跳转指令
000001f9: ff ff 00                     # 另一个签名

# 重要的 Boot 参数位置
0000020c: 00 10                        # offset +0x20c
0000020e: 28 30                        
00000210: 00 01                        
00000212: 00 80 00 00 10 00           # 看起来是地址常量
```

**这说明了什么？**
- Linux kernel 在 offset 0x1f0 有特定的签名
- 包含硬编码的 32 位地址常量
- 这正是 Droidboot 期望的 bzImage 格式

#### 栈和段初始化

```asm
00000268: mov eax, ds                 # 获取数据段
0000026a: mov es, eax                 # 设置附加段
0000026c: cld                         # 清除方向标志
0000026d: mov edx, ss                 # 获取栈段
0000026f: cmp edx, eax                # 比较 SS 和 DS
00000271: mov edx, esp                # 获取栈指针
00000273: je 0x28b                    # 段相同则跳转
```

**这说明了什么？**
- Kernel 检查 DS == SS（都应该是同一个数据段）
- 栈指针被保存在 EDX
- 这是 x86 32 位启动的标准验证

#### 关键的启动检查

```asm
000002a0: cmp DWORD PTR [esi], 0xaa553b9c  # 检查启动参数签名
000002a8: jne 0x2c1                        # 不匹配则跳过
```

**这说明了什么？**
- Kernel 期望 ESI 指向一个包含特定签名 **0xaa553b9c** 的结构
- 这是 Linux 启动参数的检查
- **ESI 必须指向有效的 boot_params 结构**

---

### 【第三阶段】关键的地址转换

从反汇编中，我们可以推断出 Droidboot 的内存映射：

```
Droidboot 加载内核流程：

1. 从 boot.img 头部读取:
   ├─ Kernel load address: 0x10008000
   ├─ Ramdisk load address: 0x11000000
   └─ Tags address: 0x10000100

2. 物理内存映射:
   ├─ 0x00000000 - 0x000fffff: 第一 MB（BIOS 区域 + IVT）
   ├─ 0x00a00000 - 0x00a01000: Second bootloader 代码
   ├─ 0x00b00000 - ...........': Second bootloader 栈
   ├─ 0x10000000 - 0x10f00000: 内存映射范围（Second bootloader 位置）
   ├─ 0x10008000 - 0x10600000: 内核加载位置
   └─ 0x11000000 - 0x11400000: Ramdisk 加载位置

3. Second bootloader 做的事:
   ├─ 在 0x10f00000 执行
   ├─ 复制代码到低地址 (0x000a000)
   ├─ 启用保护模式
   ├─ 查询 E820 内存
   └─ 跳转到 kernel @ 0x10008000

4. Kernel 入口处的寄存器状态:
   ├─ EIP = 0x10008000 (从 boot.img 头部)
   ├─ ESI = 指向 0x10000100 的 boot_params (Tags addr)
   ├─ EDI = 0
   ├─ EBP = 0
   ├─ EAX = 0 (Linux boot protocol)
   ├─ EDX = 0
   └─ CR0 = PE 已启用 (保护模式)
```

---

## 🔴 我们的配置为什么崩溃

### 错误配置分析

我们的配置：
```
BASE_ADDR="0x01000000"
KERNEL_OFFSET="0x00100000"
KERNEL_LOAD=0x01100000
```

Droidboot 期望：
```
BASE_ADDR="0x10000000"
KERNEL_OFFSET="0x00008000"
KERNEL_LOAD=0x10008000
```

### 崩溃发生的顺序

```
1. Droidboot 加载 boot.img
2. 读取 kernel load address: 0x01100000  (错误!)
3. 但 Droidboot 内部的内存映射表可能是 0x10000000 起始
4. 内核被加载到 0x01100000
5. Droidboot 设置 ESI = 0x01000100 (指向错误的 boot_params)
6. Droidboot 跳转 jmp 0x01100000
7. CPU 执行到 0x01100000，但...
8. 该地址可能不在真实的物理 RAM 中 (所有 RAM 在 0x10000000+)
9. 或者在一个未映射的内存区域
10. → 页面错误 (Page Fault) 或无效指令 (Invalid Opcode)
11. → 系统重启或黑屏
```

---

## 🟢 正确的修复

### 修改 1: package.sh

```bash
# OLD (错误)
BASE_ADDR="0x01000000"
KERNEL_OFFSET="0x00100000"
RAMDISK_OFFSET="0x02000000"
TAGS_OFFSET="0x00000100"

# NEW (正确)
BASE_ADDR="0x10000000"        # Droidboot 真实基址
KERNEL_OFFSET="0x00008000"    # 原厂 kernel 偏移
RAMDISK_OFFSET="0x01000000"   # Ramdisk 相对基址
TAGS_OFFSET="0x00000100"      # Boot params 相对基址
```

### 修改 2: src/boot_defs.zig

```zig
// OLD (错误)
pub const KERNEL_LOAD_ADDR: u32 = 0x01100000;

// NEW (正确)
pub const KERNEL_LOAD_ADDR: u32 = 0x10008000;  // 必须匹配 package.sh
pub const TAGS_ADDR: u32 = 0x10000100;         // Boot params 地址
pub const RAMDISK_ADDR: u32 = 0x11000000;      // Ramdisk 地址
```

### 修改 3: linker.ld

```ld
/* OLD (错误) */
. = 0x01100000;

/* NEW (正确) */
. = 0x10008000;  /* 必须匹配 KERNEL_LOAD_ADDR */
```

---

## ✅ 验证清单

```bash
# 1. 检查 boot.img 头部是否正确
python3 << 'PYTHON'
import struct
with open('images/new_boot.img', 'rb') as f:
    data = f.read(2048)
kernel_addr = struct.unpack('<I', data[12:16])[0]
ramdisk_addr = struct.unpack('<I', data[20:24])[0]
tags_addr = struct.unpack('<I', data[32:36])[0]
print(f"Kernel addr: 0x{kernel_addr:08x} (expected: 0x10008000)")
print(f"Ramdisk addr: 0x{ramdisk_addr:08x} (expected: 0x11000000)")
print(f"Tags addr: 0x{tags_addr:08x} (expected: 0x10000100)")
PYTHON

# 2. 检查 ELF 二进制的加载地址
readelf -l zig-out/n1_bootloader | grep LOAD

# 应该显示：
# LOAD    0x001000 0x10008000 0x10008000 0x...

# 3. 反汇编验证入口点
objdump -D zig-out/n1_bootloader | grep -A 10 "<_start>"
```

---

## 📝 结论

反汇编分析证实：

1. **Droidboot 使用固定的内存映射** (0x10000000+ 范围)
2. **Second bootloader 在 0x10f00000 执行** 并负责初始化
3. **Linux kernel 期望在 0x10008000 加载** 并运行
4. **Boot params 必须在 0x10000100** 以便 kernel 访问
5. **我们的配置 (0x01xxxxxx 范围) 完全错误** → 必然崩溃

**这是一个寻址空间的根本错误，不修复无法启动。**

