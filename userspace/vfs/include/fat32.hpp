/**
 * fat32.hpp -- FAT32 filesystem for the VFS service
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_VFS_FAT32_HPP_
#define USERSPACE_VFS_FAT32_HPP_

#include <types.hpp>
#include <fat32_structs.hpp>

namespace cassio {
namespace vfs {

constexpr u32 MAX_HANDLES = 16;
constexpr u32 MAX_PATH = 256;
constexpr u32 MAX_NAME = 256;
constexpr u32 CACHE_SIZE = 16;

struct FileHandle {
    u32 startCluster;
    u32 size;
    u32 dirCluster;     // cluster containing the directory entry
    u32 dirOffset;      // byte offset of the short entry within that cluster
    bool isDir;
    bool inUse;
};

class Fat32 {
private:
    u32 ataPid;

    // BPB-derived layout.
    u32 bytesPerSector;
    u32 sectorsPerCluster;
    u32 bytesPerCluster;
    u32 reservedSectors;
    u32 numFats;
    u32 fatSize;         // sectors per FAT
    u32 fatStartSector;
    u32 dataStartSector;
    u32 rootCluster;
    u32 totalClusters;

    // In-memory FAT table.
    u32* fat;
    u32 fatEntryCount;

    // Handle table.
    FileHandle handles[MAX_HANDLES];

    // Sector cache (LRU).
    struct CacheEntry {
        u32 lba;
        u8* data;
        u32 age;
        bool valid;
        bool dirty;
    };
    CacheEntry cache[CACHE_SIZE];
    u32 cacheAge;

    // Sector I/O (cached).
    bool readSector(u32 lba, u8* buf);
    bool writeSector(u32 lba, const u8* buf);
    CacheEntry* cacheLookup(u32 lba);
    CacheEntry* cacheEvict();
    bool cacheRead(u32 lba, u8* buf);
    bool cacheWrite(u32 lba, const u8* buf);
    void cacheInvalidate(u32 lba);

    // Cluster I/O.
    u32 clusterToLba(u32 cluster);
    bool readCluster(u32 cluster, u8* buf);
    bool writeCluster(u32 cluster, const u8* buf);

    // FAT management.
    u32 fatGet(u32 cluster);
    void fatSet(u32 cluster, u32 value);
    u32 allocateCluster();
    void freeChain(u32 startCluster);
    bool flushFat();

    // Directory helpers.
    u8 lfnChecksum(const u8* shortName);
    void nameToShort(const char* name, u8* shortName);
    bool extractLfn(const DirEntry* entries, u32 lfnStart, u32 shortIdx,
                    char* nameOut, u32 nameMax);
    bool readDirEntry(u32 dirCluster, u32 index, char* nameOut, u32 nameMax,
                      DirEntry* entryOut, u32* entryCluster, u32* entryOffset);
    bool findEntry(u32 dirCluster, const char* name, DirEntry* entryOut,
                   u32* entryCluster, u32* entryOffset);
    u32 resolvePath(const char* path, DirEntry* entryOut,
                    u32* entryCluster, u32* entryOffset);
    u32 resolveParentPath(const char* path, char* nameOut, u32 nameMax);
    bool createEntry(u32 dirCluster, const char* name, u8 attr,
                     u32* outCluster, u32* outOffset);
    bool removeEntry(u32 dirCluster, const char* name);

    // Cluster chain helpers.
    u32 clusterAtOffset(u32 startCluster, u32 byteOffset);
    u32 chainLength(u32 startCluster);

public:
    bool mount(u32 ataPid);

    // Public API matching the old Filesystem interface.
    bool createDirectory(const char* path);
    bool remove(const char* path);

    u32 open(const char* path, bool create = false);
    i32 read(u32 handle, u32 offset, u8* buf, u32 len);
    bool write(u32 handle, const u8* data, u32 len);

    bool listEntry(const char* path, u32 index, char* nameOut, u32 nameMax);
};

} // vfs
} // cassio

#endif // USERSPACE_VFS_FAT32_HPP_
