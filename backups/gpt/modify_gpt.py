#!/usr/bin/env python3
"""
Nokia N1 GPT Partition Table Modifier
方案A: 合并 boot + recovery 为 32MB 的 boot 分区

GPT 布局:
- LBA 0: Protective MBR (512 bytes)
- LBA 1: Primary GPT Header (512 bytes)
- LBA 2-33: Primary Partition Entries (128 entries × 128 bytes = 16384 bytes)
- ...数据区域...
- LBA N-33 to N-1: Secondary Partition Entries (同上)
- LBA N: Secondary GPT Header (512 bytes)

磁盘: 61071360 sectors, 最后 LBA = 61071359
"""

import struct
import sys
import os
import binascii
from copy import deepcopy

# ─── 配置 ───
SECTOR_SIZE = 512
DISK_SECTORS = 61071360
LAST_LBA = DISK_SECTORS - 1  # 61071359

# 原始分区 (LBA)
BOOT_START = 34
BOOT_END = 32801     # boot: 32768 sectors = 16 MB
RECOVERY_START = 32802
RECOVERY_END = 65569  # recovery: 32768 sectors = 16 MB

# 新布局: boot 扩展到覆盖 recovery
NEW_BOOT_END = RECOVERY_END  # 65569, boot 变为 65536 sectors = 32 MB

GPT_HEADER_OFFSET = SECTOR_SIZE  # LBA 1
GPT_ENTRIES_OFFSET = 2 * SECTOR_SIZE  # LBA 2
ENTRY_SIZE = 128
NUM_ENTRIES = 128
ENTRIES_TOTAL_SIZE = ENTRY_SIZE * NUM_ENTRIES  # 16384

def read_gpt_header(data, offset=GPT_HEADER_OFFSET):
    """解析 GPT Header"""
    hdr = data[offset:offset+92]
    fields = struct.unpack_from('<8sIIIIQQQQ16sQIII', hdr)
    return {
        'signature': fields[0],
        'revision': fields[1],
        'header_size': fields[2],
        'header_crc32': fields[3],
        'reserved': fields[4],
        'my_lba': fields[5],
        'alternate_lba': fields[6],
        'first_usable_lba': fields[7],
        'last_usable_lba': fields[8],
        'disk_guid': fields[9],
        'partition_entry_lba': fields[10],
        'num_partition_entries': fields[11],
        'partition_entry_size': fields[12],
        'partition_entries_crc32': fields[13],
    }

def read_gpt_entry(data, entry_offset):
    """解析单个 GPT 分区条目"""
    entry = data[entry_offset:entry_offset + ENTRY_SIZE]
    if len(entry) < ENTRY_SIZE:
        return None
    
    type_guid = entry[0:16]
    unique_guid = entry[16:32]
    start_lba = struct.unpack_from('<Q', entry, 32)[0]
    end_lba = struct.unpack_from('<Q', entry, 40)[0]
    attributes = struct.unpack_from('<Q', entry, 48)[0]
    name_raw = entry[56:128]
    
    # 解码 UTF-16LE 名称
    name = name_raw.decode('utf-16-le').rstrip('\x00')
    
    return {
        'type_guid': type_guid,
        'unique_guid': unique_guid,
        'start_lba': start_lba,
        'end_lba': end_lba,
        'attributes': attributes,
        'name': name,
        'raw': bytearray(entry),
    }

def pack_gpt_entry(entry):
    """将修改后的条目打包为 128 字节"""
    raw = bytearray(ENTRY_SIZE)
    raw[0:16] = entry['type_guid']
    raw[16:32] = entry['unique_guid']
    struct.pack_into('<Q', raw, 32, entry['start_lba'])
    struct.pack_into('<Q', raw, 40, entry['end_lba'])
    struct.pack_into('<Q', raw, 48, entry['attributes'])
    # 名称: UTF-16LE, 72 bytes (36 chars max)
    name_encoded = entry['name'].encode('utf-16-le')
    raw[56:56+len(name_encoded)] = name_encoded
    return bytes(raw)

def calc_crc32(data):
    """计算 CRC32 (与 GPT 规范一致)"""
    return binascii.crc32(data) & 0xFFFFFFFF

def pack_gpt_header(hdr_dict, data_template):
    """重建 GPT Header，重新计算 CRC32"""
    raw = bytearray(data_template)
    
    # 先清零 header CRC32 字段 (offset 16, 4 bytes)
    struct.pack_into('<I', raw, 16, 0)
    
    # 填入各字段
    raw[0:8] = hdr_dict['signature']
    struct.pack_into('<I', raw, 8, hdr_dict['revision'])
    struct.pack_into('<I', raw, 12, hdr_dict['header_size'])
    # CRC32 暂时为 0
    struct.pack_into('<I', raw, 20, hdr_dict['reserved'])
    struct.pack_into('<Q', raw, 24, hdr_dict['my_lba'])
    struct.pack_into('<Q', raw, 32, hdr_dict['alternate_lba'])
    struct.pack_into('<Q', raw, 40, hdr_dict['first_usable_lba'])
    struct.pack_into('<Q', raw, 48, hdr_dict['last_usable_lba'])
    raw[56:72] = hdr_dict['disk_guid']
    struct.pack_into('<Q', raw, 72, hdr_dict['partition_entry_lba'])
    struct.pack_into('<I', raw, 80, hdr_dict['num_partition_entries'])
    struct.pack_into('<I', raw, 84, hdr_dict['partition_entry_size'])
    struct.pack_into('<I', raw, 88, hdr_dict['partition_entries_crc32'])
    
    # 计算 header CRC32
    header_crc = calc_crc32(bytes(raw[:hdr_dict['header_size']]))
    struct.pack_into('<I', raw, 16, header_crc)
    
    return bytes(raw)

def guid_to_str(guid_bytes):
    """将 GUID 字节转为可读字符串"""
    parts = struct.unpack_from('<IHH', guid_bytes)
    rest = guid_bytes[8:]
    return f'{parts[0]:08x}-{parts[1]:04x}-{parts[2]:04x}-{rest[0]:02x}{rest[1]:02x}-{rest[2]:02x}{rest[3]:02x}{rest[4]:02x}{rest[5]:02x}{rest[6]:02x}{rest[7]:02x}'

def main():
    primary_file = 'gpt_primary.bin'
    secondary_file = 'gpt_secondary.bin'
    
    if not os.path.exists(primary_file) or not os.path.exists(secondary_file):
        print("错误: 找不到 GPT 备份文件")
        sys.exit(1)
    
    # ─── 读取原始数据 ───
    with open(primary_file, 'rb') as f:
        primary_data = bytearray(f.read())
    
    with open(secondary_file, 'rb') as f:
        secondary_data = bytearray(f.read())
    
    print("=" * 60)
    print("Nokia N1 GPT 分区表修改工具")
    print("方案A: 合并 boot + recovery → boot (32MB)")
    print("=" * 60)
    
    # ─── 解析主 GPT Header ───
    hdr = read_gpt_header(primary_data)
    print(f"\n[主GPT Header]")
    print(f"  签名: {hdr['signature']}")
    print(f"  My LBA: {hdr['my_lba']}")
    print(f"  Alternate LBA: {hdr['alternate_lba']}")
    print(f"  分区条目数: {hdr['num_partition_entries']}")
    print(f"  条目大小: {hdr['partition_entry_size']}")
    print(f"  条目CRC32: 0x{hdr['partition_entries_crc32']:08X}")
    print(f"  Header CRC32: 0x{hdr['header_crc32']:08X}")
    
    # ─── 验证 CRC32 ───
    # 验证分区条目 CRC32
    entries_data = primary_data[GPT_ENTRIES_OFFSET:GPT_ENTRIES_OFFSET + ENTRIES_TOTAL_SIZE]
    if len(entries_data) < ENTRIES_TOTAL_SIZE:
        print(f"\n警告: 主GPT文件不够大 ({len(primary_data)} bytes)")
        print(f"  需要至少 {GPT_ENTRIES_OFFSET + ENTRIES_TOTAL_SIZE} bytes")
        print(f"  可用分区条目数据: {len(entries_data)} bytes")
        # 使用备份GPT的分区条目
        # 备份GPT布局: entries (16384 bytes) + header (512 bytes)
        # secondary_data should be 17920 = 35 * 512 bytes
        print(f"\n使用备份GPT的分区条目 (secondary: {len(secondary_data)} bytes)")
        sec_entries = secondary_data[0:ENTRIES_TOTAL_SIZE]
        entries_data = sec_entries
    
    entries_crc = calc_crc32(bytes(entries_data))
    print(f"\n[CRC32 验证]")
    print(f"  计算的条目CRC32: 0x{entries_crc:08X}")
    print(f"  Header中的CRC32: 0x{hdr['partition_entries_crc32']:08X}")
    
    if entries_crc == hdr['partition_entries_crc32']:
        print("  ✓ 分区条目 CRC32 匹配!")
    else:
        print("  ✗ CRC32 不匹配! 检查数据完整性")
        # 尝试使用备份 GPT 的条目
        if len(secondary_data) >= ENTRIES_TOTAL_SIZE:
            sec_entries = secondary_data[0:ENTRIES_TOTAL_SIZE]
            sec_entries_crc = calc_crc32(bytes(sec_entries))
            print(f"  备份GPT条目CRC32: 0x{sec_entries_crc:08X}")
            if sec_entries_crc == hdr['partition_entries_crc32']:
                print("  ✓ 备份GPT条目匹配! 使用备份数据")
                entries_data = sec_entries
            else:
                print("  ✗ 备份GPT条目也不匹配")
    
    # ─── 解析所有分区条目 ───
    print(f"\n[当前分区表]")
    entries = []
    zero_guid = b'\x00' * 16
    for i in range(NUM_ENTRIES):
        offset = i * ENTRY_SIZE
        entry = read_gpt_entry(entries_data, offset)
        if entry is None:
            break
        if entry['type_guid'] == zero_guid:
            break
        entries.append(entry)
        size_sectors = entry['end_lba'] - entry['start_lba'] + 1
        size_mb = size_sectors * SECTOR_SIZE / (1024 * 1024)
        print(f"  [{i:2d}] {entry['name']:20s} LBA {entry['start_lba']:>10d} - {entry['end_lba']:>10d}  ({size_mb:>8.1f} MB)")
    
    print(f"\n  共 {len(entries)} 个分区")
    
    # ─── 验证 boot 和 recovery ───
    boot_entry = None
    recovery_entry = None
    recovery_idx = None
    
    for i, e in enumerate(entries):
        if e['name'] == 'boot':
            boot_entry = e
            print(f"\n[boot 分区]")
            print(f"  GUID: {guid_to_str(e['unique_guid'])}")
            print(f"  LBA: {e['start_lba']} - {e['end_lba']}")
            print(f"  大小: {(e['end_lba'] - e['start_lba'] + 1) * 512 / 1024 / 1024:.1f} MB")
        elif e['name'] == 'recovery':
            recovery_entry = e
            recovery_idx = i
            print(f"\n[recovery 分区]")
            print(f"  GUID: {guid_to_str(e['unique_guid'])}")
            print(f"  LBA: {e['start_lba']} - {e['end_lba']}")
            print(f"  大小: {(e['end_lba'] - e['start_lba'] + 1) * 512 / 1024 / 1024:.1f} MB")
    
    if not boot_entry or not recovery_entry:
        print("错误: 找不到 boot 或 recovery 分区")
        sys.exit(1)
    
    # 验证它们是相邻的
    if boot_entry['end_lba'] + 1 != recovery_entry['start_lba']:
        print(f"错误: boot 和 recovery 不相邻!")
        print(f"  boot ends at {boot_entry['end_lba']}, recovery starts at {recovery_entry['start_lba']}")
        sys.exit(1)
    
    print(f"\n✓ boot 和 recovery 分区相邻，可以合并")
    
    # ─── 修改分区表 ───
    print(f"\n{'='*60}")
    print(f"[执行修改]")
    print(f"  boot: LBA {boot_entry['start_lba']}-{boot_entry['end_lba']} → {boot_entry['start_lba']}-{NEW_BOOT_END}")
    print(f"  boot新大小: {(NEW_BOOT_END - boot_entry['start_lba'] + 1) * 512 / 1024 / 1024:.1f} MB")
    print(f"  recovery: 删除 (将被清零)")
    
    # 修改条目
    new_entries_data = bytearray(entries_data)
    
    # 1. 修改 boot 的 end_lba
    boot_offset = 0 * ENTRY_SIZE  # boot 是第一个条目
    struct.pack_into('<Q', new_entries_data, boot_offset + 40, NEW_BOOT_END)
    
    # 2. 清零 recovery 条目
    rec_offset = recovery_idx * ENTRY_SIZE
    new_entries_data[rec_offset:rec_offset + ENTRY_SIZE] = b'\x00' * ENTRY_SIZE
    
    # 但是！删除中间的条目会导致后续条目不连续
    # GPT 规范允许中间有空条目，但为了整洁，我们把后续条目前移
    # 找到 recovery 之后的条目数量
    remaining_count = len(entries) - recovery_idx - 1
    if remaining_count > 0:
        # 将 recovery 之后的所有条目前移一位
        src_start = (recovery_idx + 1) * ENTRY_SIZE
        dst_start = recovery_idx * ENTRY_SIZE
        remaining_bytes = remaining_count * ENTRY_SIZE
        new_entries_data[dst_start:dst_start + remaining_bytes] = entries_data[src_start:src_start + remaining_bytes]
        # 清零最后一个位置
        last_pos = (len(entries) - 1) * ENTRY_SIZE
        new_entries_data[last_pos:last_pos + ENTRY_SIZE] = b'\x00' * ENTRY_SIZE
        print(f"  后续 {remaining_count} 个条目已前移")
    
    # ─── 计算新的 CRC32 ───
    new_entries_crc = calc_crc32(bytes(new_entries_data))
    print(f"\n[新CRC32]")
    print(f"  旧条目CRC32: 0x{hdr['partition_entries_crc32']:08X}")
    print(f"  新条目CRC32: 0x{new_entries_crc:08X}")
    
    # ─── 构建新的主 GPT Header ───  
    new_primary_hdr = dict(hdr)
    new_primary_hdr['partition_entries_crc32'] = new_entries_crc
    # my_lba = 1, alternate_lba = 61071359, partition_entry_lba = 2 (不变)
    
    primary_hdr_raw = primary_data[GPT_HEADER_OFFSET:GPT_HEADER_OFFSET + SECTOR_SIZE]
    new_primary_hdr_packed = pack_gpt_header(new_primary_hdr, primary_hdr_raw)
    
    # ─── 构建新的备份 GPT Header ───
    # 备份 GPT Header 在 secondary_data 的最后 512 字节
    sec_hdr_offset = len(secondary_data) - SECTOR_SIZE
    sec_hdr = read_gpt_header(secondary_data, sec_hdr_offset)
    print(f"\n[备份GPT Header]")
    print(f"  My LBA: {sec_hdr['my_lba']} (应为 {LAST_LBA})")
    print(f"  Alternate LBA: {sec_hdr['alternate_lba']} (应为 1)")
    print(f"  Partition entry LBA: {sec_hdr['partition_entry_lba']}")
    
    new_sec_hdr = dict(sec_hdr)
    new_sec_hdr['partition_entries_crc32'] = new_entries_crc
    
    sec_hdr_raw = secondary_data[sec_hdr_offset:sec_hdr_offset + SECTOR_SIZE]
    new_sec_hdr_packed = pack_gpt_header(new_sec_hdr, sec_hdr_raw)
    
    # ─── 验证新的分区表 ───
    print(f"\n[验证新分区表]")
    new_entries = []
    for i in range(NUM_ENTRIES):
        offset = i * ENTRY_SIZE
        entry = read_gpt_entry(new_entries_data, offset)
        if entry is None or entry['type_guid'] == zero_guid:
            break
        new_entries.append(entry)
        size_sectors = entry['end_lba'] - entry['start_lba'] + 1
        size_mb = size_sectors * SECTOR_SIZE / (1024 * 1024)
        print(f"  [{i:2d}] {entry['name']:20s} LBA {entry['start_lba']:>10d} - {entry['end_lba']:>10d}  ({size_mb:>8.1f} MB)")
    
    print(f"\n  共 {len(new_entries)} 个分区 (原 {len(entries)} 个)")
    
    # 检查无重叠
    sorted_entries = sorted(new_entries, key=lambda e: e['start_lba'])
    for i in range(len(sorted_entries) - 1):
        if sorted_entries[i]['end_lba'] >= sorted_entries[i+1]['start_lba']:
            print(f"  ✗ 分区重叠! {sorted_entries[i]['name']} 和 {sorted_entries[i+1]['name']}")
            sys.exit(1)
    print("  ✓ 无分区重叠")
    
    # ─── 输出文件 ───
    # 1. 主 GPT Header (LBA 1, 512 bytes)
    with open('new_primary_header.bin', 'wb') as f:
        f.write(new_primary_hdr_packed)
    
    # 2. 分区条目 (LBA 2-33, 16384 bytes)
    with open('new_partition_entries.bin', 'wb') as f:
        f.write(bytes(new_entries_data))
    
    # 3. 备份 GPT Header (最后一个 LBA, 512 bytes)
    with open('new_secondary_header.bin', 'wb') as f:
        f.write(new_sec_hdr_packed)
    
    # 4. 完整的主 GPT (MBR + Header + Entries) 用于验证
    full_primary = bytearray(primary_data[:SECTOR_SIZE])  # MBR (保持不变)
    full_primary += new_primary_hdr_packed
    full_primary += bytes(new_entries_data)
    with open('new_gpt_primary_full.bin', 'wb') as f:
        f.write(full_primary)
    
    # 5. 完整的备份 GPT (Entries + Header) 用于写入
    full_secondary = bytes(new_entries_data) + new_sec_hdr_packed
    with open('new_gpt_secondary_full.bin', 'wb') as f:
        f.write(full_secondary)
    
    print(f"\n[输出文件]")
    files = [
        ('new_primary_header.bin', '主GPT Header → dd to LBA 1'),
        ('new_partition_entries.bin', '分区条目 → dd to LBA 2 (primary) 和 LBA N-33 (backup)'),
        ('new_secondary_header.bin', '备份GPT Header → dd to LBA N'),
        ('new_gpt_primary_full.bin', '完整主GPT (验证用/一次性写入)'),
        ('new_gpt_secondary_full.bin', '完整备份GPT (验证用/一次性写入)'),
    ]
    for fname, desc in files:
        size = os.path.getsize(fname)
        print(f"  {fname}: {size} bytes - {desc}")
    
    # ─── 生成写入命令 ───
    # 备份 GPT 分区条目的起始 LBA
    sec_entry_lba = sec_hdr['partition_entry_lba']
    
    print(f"\n{'='*60}")
    print("[写入命令 (在设备上执行)]")
    print(f"{'='*60}")
    print()
    print("# 步骤 0: 再次备份当前 GPT (安全措施)")
    print("dd if=/dev/block/mmcblk0 bs=512 count=34 of=/tmp/gpt_bak_pre_modify.bin")
    print(f"dd if=/dev/block/mmcblk0 bs=512 skip={LAST_LBA - 33} count=34 of=/tmp/gpt_bak2_pre_modify.bin")
    print()
    print("# 步骤 1: 写入主 GPT Header (LBA 1)")
    print("dd if=/tmp/new_primary_header.bin of=/dev/block/mmcblk0 bs=512 seek=1 count=1 conv=notrunc")
    print()
    print("# 步骤 2: 写入主分区条目 (LBA 2-33)")
    print("dd if=/tmp/new_partition_entries.bin of=/dev/block/mmcblk0 bs=512 seek=2 count=32 conv=notrunc")
    print()
    print(f"# 步骤 3: 写入备份分区条目 (LBA {sec_entry_lba})")
    print(f"dd if=/tmp/new_partition_entries.bin of=/dev/block/mmcblk0 bs=512 seek={sec_entry_lba} count=32 conv=notrunc")
    print()
    print(f"# 步骤 4: 写入备份 GPT Header (LBA {LAST_LBA})")
    print(f"dd if=/tmp/new_secondary_header.bin of=/dev/block/mmcblk0 bs=512 seek={LAST_LBA} count=1 conv=notrunc")
    print()
    print("# 步骤 5: 同步并验证")
    print("sync")
    print("blockdev --rereadpt /dev/block/mmcblk0")
    print("cat /proc/partitions | grep mmcblk0")
    
    print(f"\n{'='*60}")
    print("完成! 请仔细检查以上输出后再执行写入命令。")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()
