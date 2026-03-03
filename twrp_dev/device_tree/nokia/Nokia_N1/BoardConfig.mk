#
# Copyright (C) 2024 The TWRP Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Nokia N1 (Moorefield / mofd_v1) - TWRP Device Tree
# Stock kernel 3.10.62-x86_64_moor, 32-bit TWRP userspace

DEVICE_PATH := device/nokia/Nokia_N1

# =========================================================
# Architecture
# =========================================================
# Kernel is 64-bit x86_64, but TWRP userspace is 32-bit i386
# The stock kernel supports ia32 compat mode
TARGET_ARCH := x86
TARGET_ARCH_VARIANT := x86
TARGET_CPU_ABI := x86
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := silvermont

# 64-bit kernel support (needed for kernel module loading)
TARGET_IS_64_BIT := true
TARGET_KERNEL_ARCH := x86_64
TARGET_SUPPORTS_64_BIT_APPS := false
TARGET_SUPPORTS_32_BIT_APPS := true

# =========================================================
# Platform / Board
# =========================================================
TARGET_BOARD_PLATFORM := moorefield
TARGET_BOOTLOADER_BOARD_NAME := moorefield
TARGET_NO_BOOTLOADER := true

# =========================================================
# Kernel (Prebuilt - stock Nokia N1 kernel)
# =========================================================
# Stock kernel from build: sam@topaz4, 3.10.62-x86_64_moor, Nov 25 2015
# Do NOT compile kernel - reuse stock binary
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilt/kernel
BOARD_KERNEL_IMAGE_NAME := bzImage
BOARD_KERNEL_CMDLINE := init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 gpt bootboost=1

# Boot image parameters (verified from original TWRP boot.img)
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_OFFSET := 0x00008000
BOARD_RAMDISK_OFFSET := 0x01000000
BOARD_SECOND_OFFSET := 0x00f00000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_TAGS_OFFSET := 0x00000100

# Intel Bootstub 1.4 as second stage bootloader (REQUIRED for Nokia N1)
BOARD_SECOND_IMAGE := $(DEVICE_PATH)/prebuilt/second

# =========================================================
# Partitions
# =========================================================
# eMMC block device path (PCI, not platform)
# /dev/block/pci/pci0000:00/0000:00:01.0/by-name/<partition>
# All sizes verified via blockdev --getsize64 on device (2025-03-03)
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216         # 16 MB
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216     # 16 MB
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648     # 2 GB
BOARD_USERDATAIMAGE_PARTITION_SIZE := 26670493184  # ~24.8 GB
BOARD_CACHEIMAGE_PARTITION_SIZE := 1610612736      # 1.5 GB
BOARD_FLASH_BLOCK_SIZE := 2048

# Additional partition sizes (for reference)
# fastboot:      16777216     (16 MB)
# splashscreen:  4194304      (4 MB)
# misc:          134217728    (128 MB)
# persistent:    1048576      (1 MB)
# config:        134217728    (128 MB)
# logs:          134217728    (128 MB)
# factory:       134217728    (128 MB)

# Filesystem
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SUPPRESS_EMMC_WIPE := true

# =========================================================
# Recovery / TWRP
# =========================================================
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_RECOVERY_SWIPE := true
TARGET_RECOVERY_PIXEL_FORMAT := BGRA_8888

# Recovery fstab
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/recovery/root/etc/twrp.fstab

# =========================================================
# TWRP Configuration
# =========================================================
TW_THEME := portrait_hdpi
DEVICE_RESOLUTION := 1080x1920
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/max170xx_battery
TW_BRIGHTNESS_PATH := /sys/class/backlight/intel_backlight/brightness
TW_MAX_BRIGHTNESS := 255
TW_DEFAULT_BRIGHTNESS := 128

# Encryption (FDE with footer in factory partition)
TW_INCLUDE_CRYPTO := true
TW_CRYPTO_FS_TYPE := "ext4"
TW_CRYPTO_REAL_BLKDEV := "/dev/block/by-name/data"
TW_CRYPTO_MNT_POINT := "/data"
TW_CRYPTO_FS_OPTIONS := "nosuid,nodev,noatime,discard,barrier=1,data=ordered,noauto_da_alloc"
TW_CRYPTO_KEY_LOC := "/factory/userdata_footer"

# Filesystem support
TW_NO_EXFAT := false
TW_INCLUDE_NTFS_3G := true
TW_USE_TOOLBOX := true

# Storage
TW_NO_USB_STORAGE := false
TW_HAS_NO_RECOVERY_PARTITION := false
RECOVERY_SDCARD_ON_DATA := false
TW_EXTERNAL_STORAGE_PATH := /storage/MicroSD
TW_EXTERNAL_STORAGE_MOUNT_POINT := /storage/MicroSD

# Display driver module
TW_LOAD_VENDOR_MODULES := "tngdisp.ko"

# SELinux
TWHAVE_SELINUX := true

# Misc
TW_EXCLUDE_SUPERSU := true
TW_NO_SCREEN_BLANK := true
TW_INCLUDE_REPACKTOOLS := true
BOARD_USES_BML_OVER_MTD := false
TW_INCLUDE_FB2PNG := true

# Timezone
TARGET_RECOVERY_QCOM_RTC_FIX := false
