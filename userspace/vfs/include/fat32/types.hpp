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

#include <types.hpp>

namespace cassio {
namespace vfs {

struct __attribute__((packed)) Bpb {
    u8 jump[3];
    u8 oem[8];
    u16 bytesPerSector;
    u8 sectorsPerCluster;
    u16 reservedSectors;
    u8 numFats;
    u16 rootEntryCount;      // 0 for FAT32
    u16 totalSectors16;      // 0 for FAT32
    u8 mediaType;
    u16 fatSize16;           // 0 for FAT32
    u16 sectorsPerTrack;
    u16 numberOfHeads;
    u32 hiddenSectors;
    u32 totalSectors32;
    u32 fatSize32;
    u16 extFlags;
    u16 fsVersion;
    u32 rootCluster;
    u16 fsInfoSector;
    u16 backupBootSector;
    u8 reserved[12];
    u8 driveNumber;
    u8 reserved1;
    u8 bootSignature;
    u32 volumeSerial;
    u8 volumeLabel[11];
    u8 fsType[8];
};

struct __attribute__((packed)) DirEntry {
    u8 name[11];
    u8 attr;
    u8 ntReserved;
    u8 createTimeTenth;
    u16 createTime;
    u16 createDate;
    u16 accessDate;
    u16 firstClusterHigh;
    u16 modifyTime;
    u16 modifyDate;
    u16 firstClusterLow;
    u32 fileSize;
};

struct __attribute__((packed)) LfnEntry {
    u8 order;
    u16 name1[5];
    u8 attr;
    u8 type;
    u8 checksum;
    u16 name2[6];
    u16 firstClusterLow;  // always 0
    u16 name3[2];
};

namespace DirAttr {
    constexpr u8 ReadOnly  = 0x01;
    constexpr u8 Hidden    = 0x02;
    constexpr u8 System    = 0x04;
    constexpr u8 VolumeId  = 0x08;
    constexpr u8 Directory = 0x10;
    constexpr u8 Archive   = 0x20;
    constexpr u8 LfnMask   = ReadOnly | Hidden | System | VolumeId;
}

constexpr u32 FAT_FREE       = 0x00000000;
constexpr u32 FAT_BAD        = 0x0FFFFFF7;
constexpr u32 FAT_EOC        = 0x0FFFFFF8;
constexpr u32 FAT_ENTRY_MASK = 0x0FFFFFFF;

constexpr u8 DIR_ENTRY_FREE    = 0xE5;
constexpr u8 DIR_ENTRY_END     = 0x00;
constexpr u8 LFN_LAST_ENTRY    = 0x40;
constexpr u32 LFN_CHARS_PER_ENTRY = 13;

constexpr u32 SECTOR_SIZE = 512;
constexpr u32 ENTRIES_PER_SECTOR = SECTOR_SIZE / sizeof(DirEntry);

} // vfs
} // cassio

#endif // USERSPACE_VFS_FAT32_TYPES_HPP_
