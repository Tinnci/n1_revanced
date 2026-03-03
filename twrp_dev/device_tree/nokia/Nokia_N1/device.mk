#
# Copyright (C) 2024 The TWRP Open Source Project
#
# Device makefile for Nokia N1 (Moorefield / mofd_v1)
#

LOCAL_PATH := device/nokia/Nokia_N1

# Device identifier
PRODUCT_DEVICE := Nokia_N1
PRODUCT_NAME := omni_Nokia_N1
PRODUCT_BRAND := Nokia
PRODUCT_MODEL := N1
PRODUCT_MANUFACTURER := Nokia
PRODUCT_BOARD := moorefield

# Inherit from TWRP common config
$(call inherit-product, vendor/omni/config/common.mk)

# Device properties (from stock default.prop)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.product.device=Nokia_N1 \
    ro.product.model=N1 \
    ro.product.brand=Nokia \
    ro.product.manufacturer=Nokia \
    ro.product.board=moorefield \
    ro.board.platform=moorefield \
    ro.build.characteristics=tablet,nosdcard \
    ro.product.cpu.abi=x86_64 \
    ro.product.cpu.abilist=x86_64,x86,armeabi-v7a,armeabi \
    ro.product.cpu.abilist32=x86,armeabi-v7a,armeabi \
    ro.product.cpu.abilist64=x86_64 \
    ro.carrier=wifi-only \
    ro.zygote=zygote64_32 \
    ro.opengles.version=196608

# Recovery ramdisk extra files
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/recovery/root/etc/twrp.fstab:recovery/root/etc/twrp.fstab \
    $(LOCAL_PATH)/recovery/root/lib/modules/tngdisp.ko:recovery/root/lib/modules/tngdisp.ko \
    $(LOCAL_PATH)/recovery/root/sbin/lauchrecovery:recovery/root/sbin/lauchrecovery \
    $(LOCAL_PATH)/recovery/root/intel_prop.cfg:recovery/root/intel_prop.cfg
