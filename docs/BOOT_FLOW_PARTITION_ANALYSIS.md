# Nokia N1 启动流程与分区调整可行性分析

## 1. 完整启动链

```
┌─────────────────────────────────────────────────────────────────────┐
│  硬件复位 (Power On / Reset)                                        │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Boot ROM (片上只读) ─── 从 SPI NOR (IFWI) 加载 UMIP               │
│  [16位实模式]                                                       │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  SCU Firmware ─── 初始化 PMIC/时钟/SMIP                             │
│  [VxWorks RTOS on Lakemont core]                                    │
├─────────────────────────────────────────────────────────────────────┤
│  Chaabi Security ─── 验签固件 (CH00-CH17)                           │
│  [独立安全处理器]                                                    │
├─────────────────────────────────────────────────────────────────────┤
│  MRC (Memory Reference Code) ─── 初始化 DDR3 内存                   │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  IAFW (IA Firmware) ─── x86核心初始化, E820内存映射                  │
│  [32位保护模式]                                                      │
│                                                                     │
│  决策逻辑:                                                           │
│    1. 读取 eMMC 上的 GPT 分区表                                      │
│    2. 检查启动模式标志:                                               │
│       - 正常启动     → 加载 "boot" 分区                              │
│       - Recovery触发  → 加载 "recovery" 分区                         │
│       - Fastboot/DnX → 加载 "fastboot" 分区                         │
│       - One-shot     → 加载 "boot-one-shot" 分区                    │
│    3. 解析 ANDROID! boot image header                                │
│    4. 提取 kernel + ramdisk + second (Bootstub)                     │
│    5. 执行 Bootstub                                                  │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Bootstub 1.4 (second stage, 8KB) ─── boot.img 内嵌                │
│  [32位保护模式]                                                      │
│  - 设置 GDT, 启用 A20                                               │
│  - 传递 boot_params 给内核                                           │
│  - 跳转到 Linux 内核 32位入口                                        │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Linux Kernel 3.10.62-x86_64_moor                                   │
│  [32位入口 → 自切换到 64位长模式]                                     │
│  - 解压 bzImage, 初始化内存管理                                      │
│  - 挂载 ramdisk (initramfs)                                         │
│  - 执行 /init                                                       │
└───────────────┬─────────────────────────────────────────────────────┘
                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Android Init (ramdisk 内)                                          │
│  [64位用户空间]                                                      │
│  - 解析 init.rc / init.recovery.mofd_v1.rc                         │
│  - insmod tngdisp.ko (显示驱动)                                     │
│  - 启动 adbd / healthd / thermald                                   │
│  - 执行 recovery (TWRP)                                             │
└─────────────────────────────────────────────────────────────────────┘
```

## 2. GPT 分区表全景

eMMC: Toshiba 032GE4, 29.1GB, 61071360 sectors (512 bytes/sector)

```
LBA:  0          34        32802      65570      98338     106530     139298    172066
      ┌─────────┬─────────┬──────────┬──────────┬─────────┬──────────┬──────────┬──────
      │ GPT Hdr │  boot   │ recovery │ fastboot │ splash  │boot-one- │ ramdump  │silent
      │ (34 sec)│  16 MB  │  16 MB   │  16 MB   │  4 MB   │shot 16MB │  16 MB   │ 2MB
      └─────────┴─────────┴──────────┴──────────┴─────────┴──────────┴──────────┴──────
      [IFWI用]   [IAFW加载] [IAFW加载]  [IAFW加载]  [IAFW显示]  [IAFW加载]  [内核用]

LBA:  176162    178210    180258                   524322                  589858
      ┬─────────┬─────────┬─────────────────────────┬───────────────────────┬──────
      │userkey  │persist  │     reserved (168 MB)    │    panic (32 MB)     │factory
      │ 1 MB    │ 1 MB    │     *** 空 ***           │   dead food magic    │128 MB
      ┴─────────┴─────────┴─────────────────────────┴───────────────────────┴──────

LBA:  852008        1114152   1116200          1378344                  4524072
      ┬─────────────┬────────┬───────────────┬─────────────────────────┬──────────
      │ misc 128 MB │infork  │ config 128 MB │     cache (1.5 GB)     │logs 128MB
      │             │ 1 MB   │               │                        │
      ┴─────────────┴────────┴───────────────┴─────────────────────────┴──────────

LBA:  4786216                    8980520                              61071326
      ┬──────────────────────────┬────────────────────────────────────────┬───────
      │    system (2 GB)         │         data (24.8 GB)                 │GPT bak
      │                          │                                        │
      ┴──────────────────────────┴────────────────────────────────────────┴───────
```

## 3. 各分区当前状态

| # | 名称 | 大小 | 头部内容 | IAFW引用 | 可调整性 |
|---|------|------|---------|---------|---------|
| 1 | **boot** | 16 MB | `ANDROID!` (TWRP) | ✅ 正常启动加载 | 目标分区 |
| 2 | **recovery** | 16 MB | `ANDROID!` (recovery) | ✅ Recovery模式加载 | ⚠️ 可替换内容 |
| 3 | **fastboot** | 16 MB | `ANDROID!` (droidboot) | ✅ Fastboot模式加载 | ❌ 必须保留 |
| 4 | splashscreen | 4 MB | splash数据 | ✅ 开机画面 | ❌ 保留 |
| 5 | boot-one-shot | 16 MB | `ANDROID!` | ✅ OTA一次性启动 | ⚠️ 可借用 |
| 6 | **ramdump** | 16 MB | **全零 (空)** | ❌ 仅内核写入 | ✅ 安全缩小 |
| 7 | silentlake | 2 MB | — | ❌ Intel TXE相关 | ⚠️ 不确定 |
| 8 | userkeystore | 1 MB | — | ❌ 安全存储 | ❌ 保留 |
| 9 | persistent | 1 MB | — | ❌ 持久数据 | ❌ 保留 |
| 10 | **reserved** | **168 MB** | **全零 (空)** | ❌ 完全未使用 | ✅✅ 最佳候选 |
| 11 | panic | 32 MB | `0xDEADF00D` magic | ❌ 内核崩溃dump | ✅ 可缩小 |
| 12 | factory | 128 MB | 出厂数据 | ❌ userdata加密用 | ❌ 保留 |
| 13 | misc | 128 MB | — | ❌ bootloader通信 | ❌ 保留 |
| 14-19 | (其他) | — | — | — | 不动 |

## 4. 扩大 boot 分区的可行方案

### 方案 A: 合并 boot + recovery (★ 推荐 - 最安全)

```
当前:  boot (16MB)  |  recovery (16MB)
改后:  boot (32MB)              |  (recovery 缩为0或保留小分区)
```

**原理**: boot 和 recovery 在 GPT 中相邻 (LBA 34-32801, 32802-65569)
- 只需修改 boot 的结束 LBA: 32801 → 65569
- recovery 分区可删除或缩为 1 sector
- IAFW 通过 GPT 名称查找分区，不硬编码扇区号

**优点**:
- 相邻分区，安全扩展
- boot 变为 32MB，远超当前 ~16MB 需求
- 不影响其他分区

**缺点**:
- 失去独立 recovery 分区（但 TWRP 已在 boot 中）
- Recovery 模式触发会失败（无 recovery 分区）

**风险等级**: 低-中

### 方案 B: 从 reserved 分区借用空间

```
当前:  ..persistent(1MB) |  reserved (168MB)  | panic (32MB)..
改后:  ..persistent(1MB) |  reserved  (64MB)  |  boot2 (104MB)  | panic..
```

**原理**: reserved 分区 168MB 完全为空
- 可创建新的大分区用于存放额外数据
- 但不与 boot 相邻，不能直接扩展 boot

**优点**: 168MB 全空，超大空间
**缺点**: 不与 boot 相邻；IAFW 不认识新分区名

### 方案 C: 缩小 ramdump + reserved + panic，扩展相邻分区

较复杂的扇区级重排，风险较高。

### 方案 D: 不修改分区表，优化 ramdisk (当前方案)

```
当前 boot.img: 15.5 MB / 16 MB (余 540 KB)
```

- strace 通过 adb push 使用，不打包
- 其他工具仅占 ~110KB
- 最安全，零风险

## 5. GPT 修改技术细节

若要修改 GPT：

```bash
# 1. 备份当前 GPT
adb shell dd if=/dev/block/mmcblk0 bs=512 count=34 > gpt_primary_backup.bin
adb shell dd if=/dev/block/mmcblk0 bs=512 skip=61071325 count=35 > gpt_secondary_backup.bin

# 2. 需要工具 (sgdisk/gdisk)
# 目前设备上没有，需要编译静态版本

# 3. 修改示例 (方案A: 合并 boot+recovery)
sgdisk --delete=2 /dev/block/mmcblk0           # 删除 recovery
sgdisk --change-name=1:boot /dev/block/mmcblk0  # 确保名称
sgdisk --largest-new=0 /dev/block/mmcblk0       # 或手动设置结束扇区

# 4. 必须同时更新主 GPT (LBA 1-33) 和备份 GPT (磁盘末尾)
```

**关键风险**: 如果 GPT 写入失败或数据损坏，设备将无法启动（需 DnX 模式恢复）

## 6. 执行记录

### ✅ 方案A已成功执行: 合并 boot + recovery → 32MB boot

**执行时间**: 2025-03-03

**修改内容**:
- boot 分区: LBA 34→32801 (16MB) → LBA 34→65569 (**32MB**)
- recovery 分区: **已删除**, 后续17个条目前移填补空位
- 分区总数: 19 → 18

**验证结果**:
- Header CRC32: `0xA81974E7` ✓ 匹配
- Entries CRC32: `0x5AA1A156` ✓ 匹配
- 主 GPT (LBA 1-33): ✓ 已写入并验证
- 备份 GPT (LBA 61071327-61071359): ✓ 已写入
- 无分区重叠: ✓

**备份文件**: `backups/gpt/`
- `gpt_primary.bin` - 原始主 GPT (修改前)
- `gpt_secondary.bin` - 原始备份 GPT (修改前)
- `modify_gpt.py` - 修改脚本 (含完整验证逻辑)

**注意**: 内核分区表需要重启后才会更新 (`blockdev --rereadpt` 因设备忙而失败)。

## 7. IAFW 分区查找机制

根据 IFWI 分析，IAFW 固件通过以下流程定位 boot 分区：

1. 读取 eMMC LBA 1 的 GPT Header
2. 遍历 GPT 分区表条目 (从 LBA 2 开始)
3. **按分区名称 (UTF-16LE)** 匹配 "boot"、"recovery"、"fastboot"
4. 读取匹配条目的 Start LBA 和 End LBA
5. 从 Start LBA 读取 ANDROID! header
6. 按 header 中的偏移量提取 kernel/ramdisk/second

**重要**: IAFW **不硬编码扇区号**，仅依赖 GPT 名称查找。因此修改分区边界是安全的（只要 GPT 结构完整）。
