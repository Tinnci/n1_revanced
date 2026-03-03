#!/bin/bash
#
# Nokia N1 TWRP Build Script
# Builds TWRP recovery image using the device tree
#
# Prerequisites:
#   - Ubuntu 20.04/22.04 (or compatible)
#   - ~50GB free disk space
#   - 8GB+ RAM recommended
#   - Android build dependencies installed
#
# Usage: ./build_twrp.sh [sync|build|pack|all]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEVICE_TREE_DIR="${SCRIPT_DIR}"
WORK_DIR="${HOME}/twrp_build"
TWRP_BRANCH="twrp-5.1"  # TWRP 2.8.x era, matches Android 5.1.1

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

install_deps() {
    log_info "Installing Android build dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        git-core gnupg flex bison build-essential zip curl \
        zlib1g-dev libc6-dev-i386 libncurses5-dev lib32ncurses5-dev \
        x11proto-core-dev libx11-dev lib32z1-dev \
        libgl1-mesa-dev libxml2-utils xsltproc unzip \
        python3 python-is-python3 repo openjdk-8-jdk \
        libssl-dev bc cpio
}

sync_source() {
    log_info "Setting up TWRP build tree..."
    mkdir -p "${WORK_DIR}"
    cd "${WORK_DIR}"

    if [ ! -d ".repo" ]; then
        log_info "Initializing OmniROM repo (TWRP base)..."
        # TWRP 2.8 was based on OmniROM / Android 5.1
        # For modern TWRP 3.x, use: https://github.com/nicosys/twrp_manifest
        repo init -u https://github.com/nicosys/twrp_manifest.git \
            -b "${TWRP_BRANCH}" --depth=1
    fi

    log_info "Syncing source tree (this may take a while)..."
    repo sync -j$(nproc) --force-sync --no-clone-bundle --no-tags

    # Link device tree into build tree
    log_info "Linking Nokia N1 device tree..."
    mkdir -p "${WORK_DIR}/device/nokia"
    if [ -L "${WORK_DIR}/device/nokia/Nokia_N1" ]; then
        rm "${WORK_DIR}/device/nokia/Nokia_N1"
    fi
    ln -sf "${DEVICE_TREE_DIR}" "${WORK_DIR}/device/nokia/Nokia_N1"
    log_info "Device tree linked: ${WORK_DIR}/device/nokia/Nokia_N1"
}

build_twrp() {
    cd "${WORK_DIR}"

    log_info "Setting up build environment..."
    source build/envsetup.sh

    log_info "Selecting build target: omni_Nokia_N1-eng"
    lunch omni_Nokia_N1-eng

    log_info "Building TWRP recovery image..."
    make -j$(nproc) recoveryimage

    RECOVERY_IMG="${WORK_DIR}/out/target/product/Nokia_N1/recovery.img"
    if [ -f "${RECOVERY_IMG}" ]; then
        log_info "Build successful!"
        log_info "Recovery image: ${RECOVERY_IMG}"
        ls -la "${RECOVERY_IMG}"
    else
        log_error "Build failed - recovery.img not found"
        exit 1
    fi
}

pack_manual() {
    # Manual packing using mkbootimg (alternative to full AOSP build)
    # This allows building a boot image WITHOUT the full Android build system
    log_info "Packing boot image manually with mkbootimg..."

    KERNEL="${DEVICE_TREE_DIR}/prebuilt/kernel"
    SECOND="${DEVICE_TREE_DIR}/prebuilt/second"
    RAMDISK_DIR="${SCRIPT_DIR}/../../ramdisk_unpacked"

    if [ ! -f "${KERNEL}" ]; then
        log_error "Kernel not found at ${KERNEL}"
        exit 1
    fi
    if [ ! -f "${SECOND}" ]; then
        log_error "Bootstub not found at ${SECOND}"
        exit 1
    fi

    # Create ramdisk cpio.gz
    log_info "Creating ramdisk..."
    RAMDISK_CPIO="/tmp/nokia_n1_ramdisk.cpio.gz"
    cd "${RAMDISK_DIR}"
    find . | cpio -o -H newc 2>/dev/null | gzip -9 > "${RAMDISK_CPIO}"
    cd "${SCRIPT_DIR}"

    # Pack with mkbootimg
    # Try system mkbootimg first, fall back to Python script
    MKBOOTIMG=""
    if command -v mkbootimg &>/dev/null; then
        MKBOOTIMG="mkbootimg"
    elif [ -f "${WORK_DIR}/out/host/linux-x86/bin/mkbootimg" ]; then
        MKBOOTIMG="${WORK_DIR}/out/host/linux-x86/bin/mkbootimg"
    else
        log_warn "mkbootimg not found. Installing via pip..."
        pip3 install mkbootimg 2>/dev/null || true
        if command -v mkbootimg &>/dev/null; then
            MKBOOTIMG="mkbootimg"
        else
            log_error "Cannot find mkbootimg. Install it or use full AOSP build."
            exit 1
        fi
    fi

    OUTPUT="${SCRIPT_DIR}/../../twrp_nokia_n1.img"

    ${MKBOOTIMG} \
        --kernel "${KERNEL}" \
        --ramdisk "${RAMDISK_CPIO}" \
        --second "${SECOND}" \
        --base 0x10000000 \
        --kernel_offset 0x00008000 \
        --ramdisk_offset 0x01000000 \
        --second_offset 0x00f00000 \
        --tags_offset 0x00000100 \
        --pagesize 2048 \
        --cmdline "init=/init pci=noearly loglevel=0 vmalloc=256M androidboot.hardware=mofd_v1 watchdog.watchdog_thresh=60 gpt bootboost=1" \
        -o "${OUTPUT}"

    if [ -f "${OUTPUT}" ]; then
        log_info "Boot image created successfully!"
        log_info "Output: ${OUTPUT}"
        ls -la "${OUTPUT}"
        log_info ""
        log_info "Flash with: fastboot flash boot ${OUTPUT}"
        log_info "  or:       dd if=${OUTPUT} of=/dev/block/by-name/boot"
    else
        log_error "Failed to create boot image"
        exit 1
    fi
}

case "${1:-all}" in
    deps)
        install_deps
        ;;
    sync)
        sync_source
        ;;
    build)
        build_twrp
        ;;
    pack)
        pack_manual
        ;;
    all)
        log_info "=== Full TWRP Build Pipeline ==="
        log_info "Step 1/3: Sync source..."
        sync_source
        log_info "Step 2/3: Build TWRP..."
        build_twrp
        log_info "Step 3/3: Done!"
        ;;
    *)
        echo "Usage: $0 [deps|sync|build|pack|all]"
        echo ""
        echo "  deps  - Install build dependencies"
        echo "  sync  - Initialize and sync TWRP source tree"
        echo "  build - Build TWRP recovery image (requires synced source)"
        echo "  pack  - Manual pack with mkbootimg (no AOSP build needed)"
        echo "  all   - Full pipeline: sync + build"
        exit 1
        ;;
esac
