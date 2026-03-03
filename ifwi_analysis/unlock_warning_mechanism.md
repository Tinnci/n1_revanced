# Nokia N1 解锁警告界面与安全启动机制分析

## 1. 你看到的警告界面

```
Start>
Press POWER key
To reject this Image
Press VOLUME key

WARNING
Your device has been altered
from its factory configuration
If you were not responsible for
these changes, the security of
your device may be at risk
Reject this image to recover
your device.
```

这是 **Android Verified Boot (AVB)** 的 **"Orange State"** 警告界面。

---

## 2. 实现机制概述

### 核心组件协作

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    Nokia N1 Verified Boot Architecture                   │
└──────────────────────────────────────────────────────────────────────────┘

    ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
    │    IFWI     │────▶│   Chaabi    │────▶│   AVB App   │
    │  (SCU/RT)   │     │  Security   │     │             │
    └─────────────┘     └─────────────┘     └─────────────┘
           │                   │                   │
           │                   │                   │
           ▼                   ▼                   ▼
    ┌─────────────────────────────────────────────────────┐
    │              Device State (存储在 RPMB)             │
    │  ┌─────────────────────────────────────────────┐   │
    │  │  LOCKED (Green) | UNLOCKED (Orange) | RED   │   │
    │  └─────────────────────────────────────────────┘   │
    └─────────────────────────────────────────────────────┘
           │
           ▼
    ┌─────────────────────────────────────────────────────┐
    │              boot.img 验证决策                       │
    │  ┌─────────────────────────────────────────────┐   │
    │  │ LOCKED: 必须签名验证通过                      │   │
    │  │ UNLOCKED: 允许未签名，但显示警告              │   │
    │  └─────────────────────────────────────────────┘   │
    └─────────────────────────────────────────────────────┘
```

---

## 3. IFWI 中的关键实现

### A. Android Verified Boot (AVB) 应用

从 IFWI 字符串分析，存在以下 Chaabi 安全应用:

```
INTELAndVerBoApp          - Android Verified Boot 应用
avb_applet_init           - AVB 小程序初始化
avb_applet_invoke_cmd     - AVB 命令调用

关键函数:
AVB_read_device_state     - 读取设备锁定状态
AVB_change_device_state   - 改变设备锁定状态
AVB_verify_boot_image     - 验证 boot.img 签名
check_avb_boot_image      - 检查 boot 镜像
AVB_verify_keystore       - 验证密钥库
```

### B. 设备状态管理

设备状态存储在 **eMMC 的 RPMB (Replay Protected Memory Block)** 分区:

```
RPMB 相关函数:
rpmb_read_block           - 读取 RPMB
rpmb_write_block          - 写入 RPMB
rpmb_init                 - 初始化 RPMB
generate_oem_id_rpmb_key  - 生成 OEM RPMB 密钥

状态检查:
Invalid device state %d   - 无效设备状态
AVB_SS_ITEM_ID            - AVB 安全存储项 ID
```

### C. RnD Token (研发令牌)

IFWI 中的关键信息:
```
RT: RnD Token bit is Set..     - 研发令牌已设置
RT: Checking if ARB check passed..  - 检查反回滚
Allowed unsigned component to pass (%08x)  - 允许未签名组件通过
```

**RnD Token** 是 Intel 平台的一个特殊机制:
- 存储在 eMMC 或 UMIP 中
- 设置后允许加载未签名的 OS 镜像
- 这就是为什么解锁后可以刷自定义 ROM

---

## 4. 解锁后的验证流程

### 完整启动序列 (解锁状态)

```
1. SCU 启动
   │
   ├── RT: PMIC Scratch Reg Boot Flow  (读取启动模式)
   │
2. 读取设备状态
   │
   ├── AVB_read_device_state()         (从 RPMB 读取)
   │   └── 返回: UNLOCKED
   │
3. 读取 boot.img
   │
   ├── RT: Issuing "ROSIP" (request for OSIP)
   │
4. 验证 boot.img
   │
   ├── AVB_verify_boot_image()
   │   ├── 检查签名 → 失败 (未签名/自定义签名)
   │   ├── 检查设备状态 → UNLOCKED
   │   └── 决策: 允许启动，但显示警告
   │
5. 显示警告界面 (你看到的画面)
   │
   ├── 等待用户输入:
   │   ├── POWER 键 → 确认启动
   │   └── VOLUME 键 → 拒绝，进入 recovery/droidboot
   │
6. 继续启动或回退
```

---

## 5. 按键处理机制

### A. 在 IFWI/Bootstub 中的实现

IFWI 包含按键输入处理:
```
scove_power_btn    - 电源按钮
gpio-keys          - GPIO 按键驱动
volume_down        - 音量减
volume_up          - 音量加
fp_menu_key        - 菜单键
fp_home_key        - Home 键
fp_back_key        - 返回键
```

### B. 警告界面的按键逻辑

```c
// 伪代码 - 基于 IFWI 分析推断
enum BootDecision {
    BOOT_CONTINUE,      // 继续启动当前镜像
    BOOT_RECOVERY,      // 进入恢复模式
    BOOT_DROIDBOOT      // 进入 Droidboot (fastboot)
};

BootDecision handle_unlocked_boot_warning() {
    display_warning_screen();  // 显示警告画面
    
    while (1) {
        int key = wait_for_key_press();
        
        if (key == POWER_KEY) {
            // 用户确认接受风险
            return BOOT_CONTINUE;
        }
        else if (key == VOLUME_UP || key == VOLUME_DOWN) {
            // 用户拒绝当前镜像
            // 根据具体实现可能进入 recovery 或 droidboot
            return BOOT_DROIDBOOT;
        }
    }
}
```

### C. 为什么按 VOLUME 会进入 Droidboot?

这是 Intel 平台的 **Boot Target** 机制:

```
从 IFWI 字符串:
host_get_active_ia_boot_target   - 获取活动启动目标
boot_target_handler              - 启动目标处理器
Boot target profile for %08x not found

分区路径:
/boot      - 正常 boot 分区
/recovery  - 恢复分区

当用户在警告界面按 VOLUME 键拒绝时:
1. 当前 boot.img 被标记为 "rejected"
2. 系统回退到安全的启动目标 (recovery 或 droidboot)
3. Droidboot 本身通常是签名的，所以可以验证通过
```

---

## 6. 为什么允许未签名的 boot.img?

### Intel AVB 的设计原则

```
┌────────────────────────────────────────────────────────────────┐
│                    设备状态与验证策略                           │
├──────────────┬─────────────────────────────────────────────────┤
│ 状态         │ 验证行为                                        │
├──────────────┼─────────────────────────────────────────────────┤
│ LOCKED       │ 必须使用 OEM 密钥验证签名                       │
│ (Green)      │ 签名失败 → 拒绝启动                             │
├──────────────┼─────────────────────────────────────────────────┤
│ UNLOCKED     │ 1. 尝试验证签名                                 │
│ (Orange)     │ 2. 签名失败 → 显示警告                          │
│              │ 3. 用户确认 → 允许启动                          │
│              │ 4. 用户拒绝 → 回退到安全启动目标                │
├──────────────┼─────────────────────────────────────────────────┤
│ CORRUPTED    │ 系统被篡改，必须恢复                            │
│ (Red)        │                                                 │
└──────────────┴─────────────────────────────────────────────────┘
```

### IFWI 中的实现证据

```
RT: Translation Table Contents locked due to UNSIGNED_OR_COMP OS

这行表明:
- IFWI 检测到 OS 是 "UNSIGNED" 或 "COMPROMISED"
- 但设备处于 UNLOCKED 状态
- 所以某些安全功能被锁定 (Translation Table)
- 但允许继续启动
```

---

## 7. RPMB 存储的设备状态

### RPMB 中的 AVB 数据

```
AVB_SS_ITEM_ID   - 安全存储项 ID

存储内容包括:
1. device_state    - LOCKED/UNLOCKED/CORRUPTED
2. rollback_index  - 防回滚计数器
3. public_key_hash - 已验证的公钥哈希 (用于自定义密钥)
```

### 解锁过程 (fastboot oem unlock)

```
1. 用户在 fastboot 执行 "oem unlock"
   │
2. AVB_change_device_state(UNLOCKED)
   │
3. 写入 RPMB:
   │   - device_state = UNLOCKED
   │   - 清除用户数据 (工厂重置)
   │
4. 下次启动时:
   │   - AVB_read_device_state() 返回 UNLOCKED
   │   - 未签名镜像被允许 (带警告)
```

---

## 8. 绕过机制详解

### A. RnD Token 方式

这是 Intel 的开发者机制:

```
RT: RnD Token bit is Set..
Allowed unsigned component to pass (%08x)

RnD Token 设置后:
- 签名验证可以被跳过
- 通常由 Intel/OEM 提供给开发者
- 存储在 eMMC 的特定位置
```

### B. Userdebug/Eng 固件

`Allowed unsigned component to pass` 这个信息表明:
- 你的 IFWI 可能是 userdebug 版本
- 或者设备已经通过某种方式设置了 RnD Token

### C. Chaabi 验证链

即使允许未签名 OS，Chaabi 固件本身仍然是验证的:

```
RT: Reading partial Chaabi FW for CH01..
RT: Reading partial Chaabi FW for CH02..
RT: Security Init 1..

Chaabi 固件 (CH00-CH17) 始终需要签名验证
只有 OS 层面 (boot.img) 的验证可以被跳过
```

---

## 9. 总结

### 为什么解锁后能启动未签名 boot.img?

1. **设备状态**: `AVB_read_device_state()` 返回 `UNLOCKED`
2. **验证策略**: UNLOCKED 状态允许未签名镜像 (带警告)
3. **用户确认**: 按 POWER 键表示用户接受风险
4. **RPMB 存储**: 解锁状态持久化存储在 RPMB 中

### 按键行为

| 按键 | 行为 |
|------|------|
| **POWER** | 确认接受风险，继续启动当前 boot.img |
| **VOLUME** | 拒绝当前镜像，回退到 droidboot/recovery |

### 安全保证

即使解锁:
1. **Chaabi 固件** 仍然需要签名验证
2. **IFWI 本身** 无法被篡改
3. **某些安全功能被禁用** (如 Translation Table 锁定)
4. **用户数据** 在解锁时被清除 (工厂重置)

---

## 10. 相关 IFWI 字符串汇总

```
# 设备状态管理
AVB_read_device_state
AVB_change_device_state
Invalid device state %d

# 签名验证
AVB_verify_boot_image
check_avb_boot_image
Allowed unsigned component to pass (%08x)
Modified avb_boot_image_header 0x%x

# RnD/开发模式
RT: RnD Token bit is Set..
BYPASSED
NOT ALLOWED TO BYPASS RPMB KEY PROGRAMMING

# 启动目标
host_get_active_ia_boot_target
boot_target_handler
/boot
/recovery

# 未签名 OS 处理
RT: Translation Table Contents locked due to UNSIGNED_OR_COMP OS
```

这就是 Nokia N1 实现解锁启动和警告界面的完整机制！
