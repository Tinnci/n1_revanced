# Nokia N1 验证清单

> **⚠️ 注意**: 本文档已于 2026-03-02 更新，反映当前的 TWRP-based lk2nd 方案。  
> 之前引用的 Zig 源码（`src/boot_defs.zig`, `hw/pci_ids.zig` 等）已不存在，旧版本见 `VERIFICATION_CHECKLIST.md.old`。

---

## 1. Ground Truth 参数（从原厂镜像直接验证）

这些参数是从真实设备/原厂固件中提取的事实，不是推测。

### Boot 参数 ✅ 已验证

| 参数 | 值 | 验证来源 |
|------|-----|---------|
| Kernel load addr | `0x10008000` | `file images/original/droidboot.img` 输出 |
| Ramdisk load addr | `0x11000000` | 同上 |
| Second load addr | `0x10f00000` | 同上 |
| Page size | `2048` | 同上 |
| Cmdline (hardware) | `androidboot.hardware=mofd_v1` | 同上 |
| Kernel version | `3.10.62-x86_64_moor` | `file twrp_dev/kernel` 输出 |
| Bootstub version | `1.4` | `strings droidboot_analysis/extracted_files/second` |
| CPU | Intel Atom Z3580, 4 cores, x86_64 | 设备上 `/proc/cpuinfo` |
| 内核模式 | 64-bit 长模式 | kernel bzImage header: "legacy 64-bit entry point" |
| Droidboot 用户空间 | 64-bit (x86_64 静态 ELF) | `file` 命令确认 init, adbd 等 |
| TWRP 用户空间 | 32-bit (i386) | `file` 命令确认 init, recovery 等 |

### USB 控制器 ✅ 已验证

| 参数 | 值 | 验证来源 |
|------|-----|---------|
| PCI ID | `8086:119E` | TWRP 下 lspci/usb_dump 确认 |
| PCI Address | `00:11.0` | 同上 |
| MMIO Base (BAR0) | `0xF9100000` | usb_dump 工具输出 |
| DWC3 IP Core ID | `0x5533250A` (ver 2.50a) | 寄存器 GSNPSID 读取 |
| GCTL | `0x27102000` | usb_dump golden values |
| GUSB2PHYCFG | `0x00002600` | usb_dump golden values |
| GUSB3PIPECTL | `0x02000002` | usb_dump golden values |

### IFWI ✅ 基于二进制分析

| 参数 | 值 | 验证来源 |
|------|-----|---------|
| 固件版本 | E123.2001 | 用户提供 |
| 平台 | ANN (Anniedale) / TNG (Tangier) | IFWI 字符串分析 |
| 安全处理器 | Chaabi | IFWI 字符串 "Chaabi ANN" |
| 设备状态存储 | RPMB | IFWI 字符串 "rpmb_read_block" 等 |

---

## 2. 部分验证 / 有假设的参数

| 参数 | 值 | 可信度 | 说明 |
|------|-----|--------|------|
| GPU BAR2 基址 | `0x80000000` | ⚠️ 可能正确 | 来自 usb_dump 中 GPU inspection，但未做专门的 framebuffer 测试 |
| GPU BAR Size | 512MB | ⚠️ 估计 | 需要通过实际写入验证 |
| 分辨率 | 2048x1536 | ⚠️ 来自产品规格 | 需要 boot_params 中的 stride 验证 |
| 像素格式 | BGRA8888 | ⚠️ Android 常见 | 未通过实际 framebuffer 写入验证 |
| TSC 频率 | ~2.33 GHz | ⚠️ 名义值 | Z3580 标称频率，实际可能有变化 |
| 看门狗超时 | ~60s (可禁用) | ✅ 已验证 | `echo 1 > /sys/devices/virtual/misc/watchdog/disable` 可禁用软件看门狗 |
| UART | 无外部引出 | ⚠️ 推测 | 基于 PCB 分析，可能有测试点 |

---

## 3. 功能验证进度

### ✅ 已通过实际测试

| 功能 | 测试日期 | 验证方式 |
|------|----------|---------|
| TWRP boot.img 正常启动 | 2026-01-18 | 刷入后设备启动到 TWRP |
| ADB 连接 | 2026-01-18 | `adb shell` 正常工作 |
| kexec 命令可用 | 2026-01-18 | `kexec --help` 正常输出 |
| kexec 系统调用支持 | 2026-01-18 | 内核 30 个 kexec 符号、sysfs 接口 |
| kexec 成功加载并跳转 | 2026-01-18 | 64-bit patched kexec 执行跳转 |
| **kexec'd 内核完全启动** | 2026-03-03 | e820 @ 0x400 确认、pstore 记录了完整 561s boot log、SFI/USB/ADB 全部工作 |
| debugfs 必须挂载 | 2026-03-03 | 不挂载 debugfs 则 hardware_subarch=0 (错误) 而非 3 (INTEL_MID) |
| kexec boot 识别标志 | 2026-03-03 | kexec boot: e820[0] 从 0x400 开始；bootloader boot: 从 0x0 |
| devmem 工作 | 2026-01-18 | `devmem 0x00000000` → `0xC0001454` |
| RNDIS USB 切换 | 2026-01-18 | `setprop sys.usb.config rndis,adb` |
| RNDIS 网络连通 | 2026-01-18 | ping 192.168.100.1 100% 成功 |
| tcpsvd shell 访问 | 2026-01-18 | 端口 2525 可靠连接 |
| SELinux permissive | 2026-01-18 | `getenforce` 输出 Permissive |

### ✅ 之前误判、现已确认工作

| 功能 | 实际状态 | 确认日期 |
|------|------|------|
| kexec 跳转后启动新内核 | ✅ **完全成功！** 内核正常运行 561 秒直到手动 reboot。之前误判为失败是因为 pstore 显示上一次 boot 的日志。 | 2026-03-03 |

### ⚠️ 已知限制

| 功能 | 问题 | 日期 |
|------|------|------|
| 32-bit kexec 加载 64-bit 内核 | memfd_create 不可用 (kernel 3.10)，需要 64-bit patched kexec | 2026-03-02 |

### ❓ 未测试

| 功能 | 说明 |
|------|------|
| 主线 Linux 内核 | 需要获取/编译适配 Intel Atom 的内核 |
| UMS (USB 大容量存储) | 暴露 eMMC 分区 |
| Framebuffer 直接写入 | 验证 GPU BAR2 和像素格式 |
| GPIO / 按键 | 地址未知 |

---

## 4. 文件完整性验证

### SHA256 校验 (Ground Truth 文件)

```
# kernel (twrp_dev/kernel == droidboot_analysis/extracted_files/kernel)
d8be6cfb5b77e5be72b477f103281e817aeaf684537c228da4288a35e8dc6711

# second (twrp_dev/second == droidboot_analysis/extracted_files/second)
6eb5ac09b14165e213c09e94568585981c3f41cf5abff793c807676fc57876e3
```

如需验证：
```bash
sha256sum twrp_dev/kernel droidboot_analysis/extracted_files/kernel
sha256sum twrp_dev/second droidboot_analysis/extracted_files/second
```

---

## 5. 文档可信度

| 文档 | 可信度 | 说明 |
|------|--------|------|
| `droidboot_analysis/report.md` | ✅ 高 | 基于 `file`/`strings` 实际输出 |
| `ifwi_analysis/report.md` | ✅ 高 | 基于二进制分析 |
| `docs/BOOTLOADER_DISASSEMBLY_ANALYSIS.md` | ✅ 较高 | 基于反汇编，但解读有推测 |
| `twrp_dev/LK2ND_IMPLEMENTATION.md` | ✅ 较高 | 验证表格基于实测 |
| `twrp_dev/DEVELOPMENT_GUIDE.md` | ✅ 较高 | 实用指南 |
| `docs/HARDWARE_SPECS.md` | ⚠️ 中等 | Section 1 地址 `0x01100000` 是旧方案遗留；Section 2-4 部分参数是估计 |
| `ifwi_analysis/unlock_warning_mechanism.md` | ⚠️ 中等 | AVB 实现细节基于字符串推测 |
| `docs/REVERSE_ENGINEERING.md` | ⚠️ 中等 | Droidboot 启动流程是推断 |
| `docs/PACKAGING.md` | ✅ 已更新 | 2026-03-02 更新为当前方案 |

---

**最后更新**: 2026-03-03
