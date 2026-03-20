/**
 * fat32/filesystem.hpp -- FAT32 filesystem for the VFS service
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_VFS_FAT32_FILESYSTEM_HPP_
#define USERSPACE_VFS_FAT32_FILESYSTEM_HPP_

#include <std/ata.hpp>
#include <std/ptr.hpp>
#include <std/types.hpp>

#include <fat32/types.hpp>

namespace cassio {
namespace vfs {

constexpr std::u32 MAX_HANDLES = 16;
constexpr std::u32 MAX_PATH = 256;
constexpr std::u32 MAX_NAME = 256;
constexpr std::u32 CACHE_SIZE = 16;

struct FileHandle {
    std::u32 startCluster;
    std::u32 size;
    std::u32 dirCluster; // cluster containing the directory entry
    std::u32 dirOffset;  // byte offset of the short entry within that cluster
    bool isDir;
    bool inUse;
};

class Fat32Filesystem {
  private:
    std::ptr::Box<std::ata::Ata> ata;

    // BPB-derived layout.
    std::u32 bytesPerSector;
    std::u32 sectorsPerCluster;
    std::u32 bytesPerCluster;
    std::u32 reservedSectors;
    std::u32 numFats;
    std::u32 fatSize; // sectors per FAT
    std::u32 fatStartSector;
    std::u32 dataStartSector;
    std::u32 rootCluster;
    std::u32 totalClusters;

    // FAT read on demand via sector cache.
    std::u32 fatEntryCount;

    // Handle table (heap-allocated).
    std::ptr::Box<FileHandle[]> handles;

    // Sector cache (LRU, heap-allocated).
    struct CacheEntry {
        std::u32 lba;
        std::u8* data;
        std::u32 age;
        bool valid;
        bool dirty;
    };
    std::ptr::Box<CacheEntry[]> cache;
    std::u32 cacheAge;

    // Sector I/O (cached).
    bool readSector(std::u32 lba, std::u8* buf);
    bool writeSector(std::u32 lba, const std::u8* buf);
    CacheEntry* cacheLookup(std::u32 lba);
    CacheEntry* cacheEvict();
    bool cacheRead(std::u32 lba, std::u8* buf);
    bool cacheWrite(std::u32 lba, const std::u8* buf);
    // Cluster I/O.
    std::u32 clusterToLba(std::u32 cluster);
    bool readCluster(std::u32 cluster, std::u8* buf);
    bool writeCluster(std::u32 cluster, const std::u8* buf);

    // FAT management.
    std::u32 fatGet(std::u32 cluster);
    void fatSet(std::u32 cluster, std::u32 value);
    std::u32 allocateCluster();
    void freeChain(std::u32 startCluster);
    bool flushFat();

    // Directory helpers.
    std::u8 lfnChecksum(const std::u8* shortName);
    void nameToShort(const char* name, std::u8* shortName);
    bool extractLfn(const DirEntry* entries, std::u32 lfnStart, std::u32 shortIdx, char* nameOut,
                    std::u32 nameMax);
    bool readDirEntry(std::u32 dirCluster, std::u32 index, char* nameOut, std::u32 nameMax,
                      DirEntry* entryOut, std::u32* entryCluster, std::u32* entryOffset);
    bool findEntry(std::u32 dirCluster, const char* name, DirEntry* entryOut,
                   std::u32* entryCluster, std::u32* entryOffset);
    bool resolvePath(const char* path, std::u32* clusterOut, DirEntry* entryOut,
                     std::u32* entryCluster, std::u32* entryOffset);
    bool resolveParentPath(const char* path, std::u32* parentClusterOut, char* nameOut,
                           std::u32 nameMax);
    bool createEntry(std::u32 dirCluster, const char* name, std::u8 attr, std::u32* outCluster,
                     std::u32* outOffset);
    bool removeEntry(std::u32 dirCluster, const char* name);

    // Cluster chain helpers.
    std::u32 clusterAtOffset(std::u32 startCluster, std::u32 byteOffset);

  public:
    Fat32Filesystem() = default;

    bool mount();

    bool createDirectory(const char* path);
    bool remove(const char* path);

    std::u32 open(const char* path, bool create = false);
    std::i32 read(std::u32 handle, std::u32 offset, std::u8* buf, std::u32 len);
    bool write(std::u32 handle, const std::u8* data, std::u32 len);

    bool listEntry(const char* path, std::u32 index, char* nameOut, std::u32 nameMax);

    // Returns 0 = not found, 1 = file, 2 = directory.
    std::u32 stat(const char* path);

    Fat32Filesystem(const Fat32Filesystem&) = delete;
    Fat32Filesystem(Fat32Filesystem&&) = delete;
    Fat32Filesystem& operator=(const Fat32Filesystem&) = delete;
    Fat32Filesystem& operator=(Fat32Filesystem&&) = delete;
};

} // namespace vfs
} // namespace cassio

#endif // USERSPACE_VFS_FAT32_FILESYSTEM_HPP_
