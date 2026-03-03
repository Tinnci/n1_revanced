#!/bin/bash
set -e

# 下载
if [ ! -f kexec-tools-2.0.28.tar.xz ]; then
    curl -LO https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kexec/kexec-tools-2.0.28.tar.xz
fi

# 解压 (如果是全新的)
rm -rf kexec-tools-2.0.28
tar xf kexec-tools-2.0.28.tar.xz
cd kexec-tools-2.0.28

# Patch: 修复 i386 构建时的 libgen.h 缺失 (虽然我们要编译 x86_64，但保留这个修复无害)
sed -i '33 a #include <libgen.h>' kexec/arch/i386/x86-linux-setup.c

# Patch: 移除不兼容的链接器参数
sed -i 's/-Wl,-Map=$(PURGATORY_MAP)//' purgatory/Makefile

# 编译 x86_64 静态版本
# 注意: 我们不需要指定 -target i386，因为我们要的是 x86_64
echo "Configuring for x86_64-linux-musl..."
CC="zig cc -target x86_64-linux-musl -fuse-ld=lld" \
CFLAGS="-static -Os" \
LDFLAGS="-static" \
./configure --host=x86_64-linux-musl --without-xen --without-lzma --without-zlib --sbindir=/sbin

echo "Building..."
make -j$(nproc)

echo "Build complete!"
file build/sbin/kexec
