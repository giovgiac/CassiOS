/**
 * fat32/types.hpp -- FAT32 on-disk structure definitions
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_VFS_FAT32_TYPES_HPP_
#define USERSPACE_VFS_FAT32_TYPES_HPP_

#include <std/types.hpp>

namespace cassio {
namespace vfs {

struct __attribute__((packed)) Bpb {
    std::u8 jump[3];
    std::u8 oem[8];
    std::u16 bytesPerSector;
    std::u8 sectorsPerCluster;
    std::u16 reservedSectors;
    std::u8 numFats;
    std::u16 rootEntryCount; // 0 for FAT32
    std::u16 totalSectors16; // 0 for FAT32
    std::u8 mediaType;
    std::u16 fatSize16; // 0 for FAT32
    std::u16 sectorsPerTrack;
    std::u16 numberOfHeads;
    std::u32 hiddenSectors;
    std::u32 totalSectors32;
    std::u32 fatSize32;
    std::u16 extFlags;
    std::u16 fsVersion;
    std::u32 rootCluster;
    std::u16 fsInfoSector;
    std::u16 backupBootSector;
    std::u8 reserved[12];
    std::u8 driveNumber;
    std::u8 reserved1;
    std::u8 bootSignature;
    std::u32 volumeSerial;
    std::u8 volumeLabel[11];
    std::u8 fsType[8];
};

struct __attribute__((packed)) DirEntry {
    std::u8 name[11];
    std::u8 attr;
    std::u8 ntReserved;
    std::u8 createTimeTenth;
    std::u16 createTime;
    std::u16 createDate;
    std::u16 accessDate;
    std::u16 firstClusterHigh;
    std::u16 modifyTime;
    std::u16 modifyDate;
    std::u16 firstClusterLow;
    std::u32 fileSize;
};

struct __attribute__((packed)) LfnEntry {
    std::u8 order;
    std::u16 name1[5];
    std::u8 attr;
    std::u8 type;
    std::u8 checksum;
    std::u16 name2[6];
    std::u16 firstClusterLow; // always 0
    std::u16 name3[2];
};

namespace DirAttr {
constexpr std::u8 ReadOnly = 0x01;
constexpr std::u8 Hidden = 0x02;
constexpr std::u8 System = 0x04;
constexpr std::u8 VolumeId = 0x08;
constexpr std::u8 Directory = 0x10;
constexpr std::u8 Archive = 0x20;
constexpr std::u8 LfnMask = ReadOnly | Hidden | System | VolumeId;
} // namespace DirAttr

constexpr std::u32 FAT_FREE = 0x00000000;
constexpr std::u32 FAT_BAD = 0x0FFFFFF7;
constexpr std::u32 FAT_EOC = 0x0FFFFFF8;
constexpr std::u32 FAT_ENTRY_MASK = 0x0FFFFFFF;

constexpr std::u8 DIR_ENTRY_FREE = 0xE5;
constexpr std::u8 DIR_ENTRY_END = 0x00;
constexpr std::u8 LFN_LAST_ENTRY = 0x40;
constexpr std::u32 LFN_CHARS_PER_ENTRY = 13;

constexpr std::u32 SECTOR_SIZE = 512;
constexpr std::u32 ENTRIES_PER_SECTOR = SECTOR_SIZE / sizeof(DirEntry);

} // namespace vfs
} // namespace cassio

#endif // USERSPACE_VFS_FAT32_TYPES_HPP_
