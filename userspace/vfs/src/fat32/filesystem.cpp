/**
 * fat32/filesystem.cpp -- FAT32 filesystem implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <fat32/filesystem.hpp>
#include <ata_client.hpp>
#include <userheap.hpp>
#include <string.hpp>
#include <memory.hpp>

using namespace cassio;
using namespace cassio::vfs;

// ---------------------------------------------------------------------------
// Sector cache
// ---------------------------------------------------------------------------

Fat32Filesystem::CacheEntry* Fat32Filesystem::cacheLookup(u32 lba) {
    for (u32 i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].lba == lba) {
            cache[i].age = ++cacheAge;
            return &cache[i];
        }
    }
    return nullptr;
}

Fat32Filesystem::CacheEntry* Fat32Filesystem::cacheEvict() {
    // Find an empty slot first.
    for (u32 i = 0; i < CACHE_SIZE; i++) {
        if (!cache[i].valid) {
            if (!cache[i].data) {
                cache[i].data = static_cast<u8*>(UserHeap::alloc(SECTOR_SIZE));
                if (!cache[i].data) return nullptr;
            }
            return &cache[i];
        }
    }

    // Evict the least recently used entry.
    u32 oldest = 0;
    for (u32 i = 1; i < CACHE_SIZE; i++) {
        if (cache[i].age < cache[oldest].age) oldest = i;
    }

    CacheEntry* entry = &cache[oldest];
    // Flush dirty entry before evicting.
    if (entry->dirty) {
        AtaClient::writeSector(ataPid, entry->lba, entry->data);
        entry->dirty = false;
    }
    entry->valid = false;
    return entry;
}

bool Fat32Filesystem::cacheRead(u32 lba, u8* buf) {
    CacheEntry* entry = cacheLookup(lba);
    if (entry) {
        memcpy(buf, entry->data, SECTOR_SIZE);
        return true;
    }

    // Cache miss -- read from disk and cache it.
    entry = cacheEvict();
    if (!entry) return AtaClient::readSector(ataPid, lba, buf);

    if (!AtaClient::readSector(ataPid, lba, entry->data)) return false;

    entry->lba = lba;
    entry->valid = true;
    entry->dirty = false;
    entry->age = ++cacheAge;
    memcpy(buf, entry->data, SECTOR_SIZE);
    return true;
}

bool Fat32Filesystem::cacheWrite(u32 lba, const u8* buf) {
    CacheEntry* entry = cacheLookup(lba);
    if (!entry) {
        entry = cacheEvict();
        if (!entry) return AtaClient::writeSector(ataPid, lba, buf);
    }

    memcpy(entry->data, buf, SECTOR_SIZE);
    entry->lba = lba;
    entry->valid = true;
    entry->dirty = true;
    entry->age = ++cacheAge;

    // Write through to disk immediately for durability.
    return AtaClient::writeSector(ataPid, lba, buf);
}

// ---------------------------------------------------------------------------
// Sector / cluster I/O
// ---------------------------------------------------------------------------

bool Fat32Filesystem::readSector(u32 lba, u8* buf) {
    return cacheRead(lba, buf);
}

bool Fat32Filesystem::writeSector(u32 lba, const u8* buf) {
    return cacheWrite(lba, buf);
}

u32 Fat32Filesystem::clusterToLba(u32 cluster) {
    return dataStartSector + (cluster - 2) * sectorsPerCluster;
}

bool Fat32Filesystem::readCluster(u32 cluster, u8* buf) {
    u32 lba = clusterToLba(cluster);
    for (u32 i = 0; i < sectorsPerCluster; i++) {
        if (!readSector(lba + i, buf + i * SECTOR_SIZE)) return false;
    }
    return true;
}

bool Fat32Filesystem::writeCluster(u32 cluster, const u8* buf) {
    u32 lba = clusterToLba(cluster);
    for (u32 i = 0; i < sectorsPerCluster; i++) {
        if (!writeSector(lba + i, buf + i * SECTOR_SIZE)) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// FAT management
// ---------------------------------------------------------------------------

u32 Fat32Filesystem::fatGet(u32 cluster) {
    if (cluster >= fatEntryCount) return FAT_EOC;
    u32 byteOffset = cluster * 4;
    u32 sector = fatStartSector + (byteOffset / SECTOR_SIZE);
    u32 offset = byteOffset % SECTOR_SIZE;

    u8 buf[SECTOR_SIZE];
    if (!readSector(sector, buf)) return FAT_EOC;

    u32 value;
    memcpy(&value, buf + offset, 4);
    return value & FAT_ENTRY_MASK;
}

void Fat32Filesystem::fatSet(u32 cluster, u32 value) {
    if (cluster >= fatEntryCount) return;
    u32 byteOffset = cluster * 4;
    u32 sector = fatStartSector + (byteOffset / SECTOR_SIZE);
    u32 offset = byteOffset % SECTOR_SIZE;

    u8 buf[SECTOR_SIZE];
    if (!readSector(sector, buf)) return;

    u32 existing;
    memcpy(&existing, buf + offset, 4);
    u32 newVal = (existing & ~FAT_ENTRY_MASK) | (value & FAT_ENTRY_MASK);
    memcpy(buf + offset, &newVal, 4);

    writeSector(sector, buf);
}

u32 Fat32Filesystem::allocateCluster() {
    for (u32 i = 2; i < fatEntryCount; i++) {
        if (fatGet(i) == FAT_FREE) {
            fatSet(i, FAT_EOC);
            return i;
        }
    }
    return 0;
}

void Fat32Filesystem::freeChain(u32 startCluster) {
    u32 cluster = startCluster;
    while (cluster >= 2 && cluster < fatEntryCount) {
        u32 next = fatGet(cluster);
        fatSet(cluster, FAT_FREE);
        if (next >= FAT_EOC) break;
        cluster = next;
    }
}

bool Fat32Filesystem::flushFat() {
    // fatSet already writes through to FAT1 via the cache.
    // Mirror dirty FAT1 sectors to FAT2.
    for (u32 i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].dirty) {
            u32 lba = cache[i].lba;
            // Check if this is a FAT1 sector.
            if (lba >= fatStartSector && lba < fatStartSector + fatSize) {
                u32 fat2Lba = lba + fatSize;
                AtaClient::writeSector(ataPid, fat2Lba, cache[i].data);
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Cluster chain helpers
// ---------------------------------------------------------------------------

u32 Fat32Filesystem::clusterAtOffset(u32 startCluster, u32 byteOffset) {
    u32 clustersToSkip = byteOffset / bytesPerCluster;
    u32 cluster = startCluster;
    for (u32 i = 0; i < clustersToSkip; i++) {
        cluster = fatGet(cluster);
        if (cluster >= FAT_EOC) return 0;
    }
    return cluster;
}


// ---------------------------------------------------------------------------
// Directory helpers
// ---------------------------------------------------------------------------

u8 Fat32Filesystem::lfnChecksum(const u8* shortName) {
    u8 sum = 0;
    for (u32 i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + shortName[i];
    }
    return sum;
}

static bool isLfnEntry(const DirEntry* e) {
    return e->attr == DirAttr::LfnMask;
}

// Convert a readable name to 8.3 short name format.
// For simplicity, generates a unique short name using a tilde (~) suffix.
void Fat32Filesystem::nameToShort(const char* name, u8* shortName) {
    memset(shortName, ' ', 11);

    // Find the last dot for extension.
    i32 lastDot = -1;
    u32 nameLen = strlen(name);
    for (u32 i = 0; i < nameLen; i++) {
        if (name[i] == '.') lastDot = static_cast<i32>(i);
    }

    // Fill the base name (first 8 chars before the dot).
    u32 baseLen = (lastDot >= 0) ? static_cast<u32>(lastDot) : nameLen;
    u32 j = 0;
    for (u32 i = 0; i < baseLen && j < 8; i++) {
        char c = name[i];
        if (c == ' ' || c == '.') continue;
        if (c >= 'a' && c <= 'z') c -= 32;
        shortName[j++] = static_cast<u8>(c);
    }

    // Truncate with ~1 if name was too long.
    if (baseLen > 8 || lastDot >= 0) {
        if (j > 6) j = 6;
        shortName[j] = '~';
        shortName[j + 1] = '1';
    }

    // Fill the extension.
    if (lastDot >= 0) {
        u32 extStart = static_cast<u32>(lastDot) + 1;
        u32 k = 0;
        for (u32 i = extStart; i < nameLen && k < 3; i++) {
            char c = name[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            shortName[8 + k] = static_cast<u8>(c);
            k++;
        }
    }
}

bool Fat32Filesystem::extractLfn(const DirEntry* entries, u32 lfnStart, u32 shortIdx,
                       char* nameOut, u32 nameMax) {
    u32 pos = 0;
    // LFN entries are stored in reverse order before the short entry.
    // The entry closest to the short entry is sequence #1.
    for (u32 i = shortIdx; i > lfnStart; i--) {
        const LfnEntry* lfn = reinterpret_cast<const LfnEntry*>(&entries[i - 1]);
        for (u32 j = 0; j < 5 && pos < nameMax - 1; j++) {
            u16 c = lfn->name1[j];
            if (c == 0x0000 || c == 0xFFFF) { nameOut[pos] = '\0'; return true; }
            nameOut[pos++] = static_cast<char>(c & 0xFF);
        }
        for (u32 j = 0; j < 6 && pos < nameMax - 1; j++) {
            u16 c = lfn->name2[j];
            if (c == 0x0000 || c == 0xFFFF) { nameOut[pos] = '\0'; return true; }
            nameOut[pos++] = static_cast<char>(c & 0xFF);
        }
        for (u32 j = 0; j < 2 && pos < nameMax - 1; j++) {
            u16 c = lfn->name3[j];
            if (c == 0x0000 || c == 0xFFFF) { nameOut[pos] = '\0'; return true; }
            nameOut[pos++] = static_cast<char>(c & 0xFF);
        }
    }
    nameOut[pos] = '\0';
    return true;
}

// Extract the short name (8.3) into a readable string.
static void shortNameToString(const u8* shortName, char* out, u32 outMax) {
    u32 pos = 0;
    // Base name: up to 8 chars, strip trailing spaces.
    for (u32 i = 0; i < 8 && pos < outMax - 1; i++) {
        if (shortName[i] == ' ') break;
        out[pos++] = static_cast<char>(shortName[i]);
    }
    // Extension: up to 3 chars, strip trailing spaces.
    if (shortName[8] != ' ') {
        out[pos++] = '.';
        for (u32 i = 8; i < 11 && pos < outMax - 1; i++) {
            if (shortName[i] == ' ') break;
            out[pos++] = static_cast<char>(shortName[i]);
        }
    }
    out[pos] = '\0';
}

static bool nameEquals(const char* a, const char* b) {
    for (u32 i = 0; ; i++) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
    }
}

// Read the Nth directory entry from a directory cluster chain.
// Returns the entry name (from LFN or short name), the raw DirEntry,
// and the on-disk location (cluster + byte offset).
bool Fat32Filesystem::readDirEntry(u32 dirCluster, u32 index, char* nameOut,
                         u32 nameMax, DirEntry* entryOut,
                         u32* entryCluster, u32* entryOffset) {
    u8* clusterBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
    if (!clusterBuf) return false;

    // Buffer to collect LFN entries across cluster boundaries.
    // 20 entries covers 260 chars (20 * 13), enough for MAX_NAME.
    constexpr u32 MAX_LFN_ENTRIES = 20;
    DirEntry lfnBuf[MAX_LFN_ENTRIES];
    u32 lfnCount = 0;

    u32 cluster = dirCluster;
    u32 visibleIdx = 0;

    while (cluster >= 2 && cluster < FAT_EOC) {
        if (!readCluster(cluster, clusterBuf)) {
            UserHeap::free(clusterBuf);
            return false;
        }

        u32 entriesPerCluster = bytesPerCluster / sizeof(DirEntry);
        DirEntry* entries = reinterpret_cast<DirEntry*>(clusterBuf);

        for (u32 i = 0; i < entriesPerCluster; i++) {
            if (entries[i].name[0] == DIR_ENTRY_END) {
                UserHeap::free(clusterBuf);
                return false;
            }
            if (entries[i].name[0] == DIR_ENTRY_FREE) {
                lfnCount = 0;
                continue;
            }

            if (isLfnEntry(&entries[i])) {
                if (lfnCount < MAX_LFN_ENTRIES) {
                    memcpy(&lfnBuf[lfnCount], &entries[i], sizeof(DirEntry));
                    lfnCount++;
                }
                continue;
            }

            // Skip volume label and dot entries.
            if (entries[i].attr & DirAttr::VolumeId) {
                lfnCount = 0;
                continue;
            }
            if (entries[i].name[0] == '.' &&
                (entries[i].name[1] == ' ' || entries[i].name[1] == '.')) {
                lfnCount = 0;
                continue;
            }

            // This is a visible short entry.
            if (visibleIdx == index) {
                if (lfnCount > 0) {
                    // Extract LFN from the collected buffer.
                    // lfnBuf[0..lfnCount-1] are the LFN entries in order,
                    // followed by the short entry at position lfnCount.
                    memcpy(&lfnBuf[lfnCount], &entries[i], sizeof(DirEntry));
                    extractLfn(lfnBuf, 0, lfnCount, nameOut, nameMax);
                } else {
                    shortNameToString(entries[i].name, nameOut, nameMax);
                }
                memcpy(entryOut, &entries[i], sizeof(DirEntry));
                *entryCluster = cluster;
                *entryOffset = i * sizeof(DirEntry);
                UserHeap::free(clusterBuf);
                return true;
            }

            visibleIdx++;
            lfnCount = 0;
        }

        cluster = fatGet(cluster);
    }

    UserHeap::free(clusterBuf);
    return false;
}

bool Fat32Filesystem::findEntry(u32 dirCluster, const char* name, DirEntry* entryOut,
                      u32* entryCluster, u32* entryOffset) {
    char entryName[MAX_NAME];
    for (u32 i = 0; ; i++) {
        DirEntry entry;
        u32 ec, eo;
        if (!readDirEntry(dirCluster, i, entryName, MAX_NAME, &entry, &ec, &eo)) {
            return false;
        }
        if (nameEquals(entryName, name)) {
            memcpy(entryOut, &entry, sizeof(DirEntry));
            *entryCluster = ec;
            *entryOffset = eo;
            return true;
        }
    }
}

u32 Fat32Filesystem::resolvePath(const char* path, DirEntry* entryOut,
                       u32* entryCluster, u32* entryOffset) {
    if (!path || path[0] == '\0') return 0;

    u32 cluster = rootCluster;
    u32 i = 0;

    if (path[0] == '/') {
        i = 1;
        if (path[1] == '\0') {
            // Root directory -- fill a synthetic entry.
            if (entryOut) {
                memset(entryOut, 0, sizeof(DirEntry));
                entryOut->attr = DirAttr::Directory;
                entryOut->firstClusterHigh = static_cast<u16>(rootCluster >> 16);
                entryOut->firstClusterLow = static_cast<u16>(rootCluster);
            }
            return rootCluster;
        }
    }

    char component[MAX_NAME];
    while (path[i] != '\0') {
        u32 j = 0;
        while (path[i] != '\0' && path[i] != '/' && j < MAX_NAME - 1) {
            component[j++] = path[i++];
        }
        component[j] = '\0';
        if (path[i] == '/') i++;
        if (j == 0) continue;

        if (streq(component, ".")) {
            continue;
        }

        if (streq(component, "..")) {
            if (cluster == rootCluster) continue;  // already at root

            // Read the ".." entry from this directory to get the parent.
            u8 buf[SECTOR_SIZE];
            u32 lba = clusterToLba(cluster);
            if (!readSector(lba, buf)) return 0;

            DirEntry* entries = reinterpret_cast<DirEntry*>(buf);
            // ".." is always the second entry in a directory.
            u32 parentCluster =
                (static_cast<u32>(entries[1].firstClusterHigh) << 16) |
                 static_cast<u32>(entries[1].firstClusterLow);

            // FAT32 stores 0 for the root directory's cluster in "..".
            cluster = (parentCluster == 0) ? rootCluster : parentCluster;

            // Fill a synthetic entry for the parent directory.
            if (entryOut) {
                memset(entryOut, 0, sizeof(DirEntry));
                entryOut->attr = DirAttr::Directory;
                entryOut->firstClusterHigh = static_cast<u16>(cluster >> 16);
                entryOut->firstClusterLow = static_cast<u16>(cluster);
            }
            continue;
        }

        DirEntry entry;
        u32 ec, eo;
        if (!findEntry(cluster, component, &entry, &ec, &eo)) {
            return 0;
        }

        cluster = (static_cast<u32>(entry.firstClusterHigh) << 16) |
                   static_cast<u32>(entry.firstClusterLow);

        if (entryOut) memcpy(entryOut, &entry, sizeof(DirEntry));
        if (entryCluster) *entryCluster = ec;
        if (entryOffset) *entryOffset = eo;
    }

    return cluster;
}

u32 Fat32Filesystem::resolveParentPath(const char* path, char* nameOut, u32 nameMax) {
    // Find last '/'.
    i32 lastSlash = -1;
    for (u32 i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') lastSlash = static_cast<i32>(i);
    }

    if (lastSlash < 0) return 0;

    if (lastSlash == 0) {
        // Parent is root.
        u32 j = 0;
        u32 start = 1;
        while (path[start] != '\0' && j < nameMax - 1) {
            nameOut[j++] = path[start++];
        }
        nameOut[j] = '\0';
        return rootCluster;
    }

    char parentPath[MAX_PATH];
    u32 k = 0;
    while (k < MAX_PATH - 1 && k < static_cast<u32>(lastSlash)) {
        parentPath[k] = path[k];
        k++;
    }
    parentPath[k] = '\0';

    // Copy the name part.
    u32 nameStart = static_cast<u32>(lastSlash) + 1;
    u32 j = 0;
    while (path[nameStart] != '\0' && j < nameMax - 1) {
        nameOut[j++] = path[nameStart++];
    }
    nameOut[j] = '\0';

    DirEntry parentEntry;
    u32 parentCluster = resolvePath(parentPath, &parentEntry, nullptr, nullptr);
    if (parentCluster == 0) return 0;
    if (!(parentEntry.attr & DirAttr::Directory)) return 0;

    return parentCluster;
}

bool Fat32Filesystem::createEntry(u32 dirCluster, const char* name, u8 attr,
                        u32* outCluster, u32* outOffset) {
    // Generate short name.
    u8 shortName[11];
    nameToShort(name, shortName);
    u8 checksum = lfnChecksum(shortName);

    // Calculate LFN entries needed.
    u32 nameLen = strlen(name);
    u32 lfnCount = (nameLen + LFN_CHARS_PER_ENTRY - 1) / LFN_CHARS_PER_ENTRY;

    // Check if we even need LFN entries.
    // If the name fits in 8.3 and is all uppercase/no special chars, skip LFN.
    bool needsLfn = false;
    if (nameLen > 12) {
        needsLfn = true;
    } else {
        // Check if it's a valid 8.3 name.
        i32 dot = -1;
        for (u32 i = 0; i < nameLen; i++) {
            if (name[i] == '.') {
                if (dot >= 0) { needsLfn = true; break; }
                dot = static_cast<i32>(i);
            } else if (name[i] >= 'a' && name[i] <= 'z') {
                needsLfn = true;
                break;
            } else if (name[i] == ' ') {
                needsLfn = true;
                break;
            }
        }
        if (!needsLfn) {
            u32 baseLen = (dot >= 0) ? static_cast<u32>(dot) : nameLen;
            u32 extLen = (dot >= 0) ? nameLen - static_cast<u32>(dot) - 1 : 0;
            if (baseLen > 8 || extLen > 3) needsLfn = true;
        }
    }

    u32 totalEntries = (needsLfn ? lfnCount : 0) + 1;

    // Find a contiguous run of free entries in the directory.
    u8* clusterBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
    if (!clusterBuf) return false;

    u32 cluster = dirCluster;
    u32 freeRun = 0;
    u32 freeStartCluster = 0;
    u32 freeStartIdx = 0;

    while (cluster >= 2 && cluster < FAT_EOC) {
        if (!readCluster(cluster, clusterBuf)) {
            UserHeap::free(clusterBuf);
            return false;
        }

        u32 entriesPerCluster = bytesPerCluster / sizeof(DirEntry);
        DirEntry* entries = reinterpret_cast<DirEntry*>(clusterBuf);

        for (u32 i = 0; i < entriesPerCluster; i++) {
            if (entries[i].name[0] == DIR_ENTRY_END ||
                entries[i].name[0] == DIR_ENTRY_FREE) {
                if (freeRun == 0) {
                    freeStartCluster = cluster;
                    freeStartIdx = i;
                }
                freeRun++;
                if (freeRun >= totalEntries) {
                    goto found;
                }
            } else {
                freeRun = 0;
            }
        }

        u32 nextCluster = fatGet(cluster);
        if (nextCluster >= FAT_EOC) {
            // Extend the directory by allocating a new cluster.
            u32 newCluster = allocateCluster();
            if (newCluster == 0) {
                UserHeap::free(clusterBuf);
                return false;
            }
            fatSet(cluster, newCluster);
            fatSet(newCluster, FAT_EOC);
            flushFat();

            // Zero out the new cluster.
            memset(clusterBuf, 0, bytesPerCluster);
            writeCluster(newCluster, clusterBuf);

            nextCluster = newCluster;
        }
        cluster = nextCluster;
    }

    UserHeap::free(clusterBuf);
    return false;

found:
    // We have a contiguous run starting at freeStartCluster:freeStartIdx.
    // Write entries. We might need to re-read the starting cluster.
    if (!readCluster(freeStartCluster, clusterBuf)) {
        UserHeap::free(clusterBuf);
        return false;
    }

    DirEntry* entries = reinterpret_cast<DirEntry*>(clusterBuf);
    u32 writeIdx = freeStartIdx;
    u32 writeCluster_ = freeStartCluster;
    u32 entriesPerCluster = bytesPerCluster / sizeof(DirEntry);

    // Write LFN entries.
    if (needsLfn) {
        for (u32 seq = lfnCount; seq >= 1; seq--) {
            LfnEntry* lfn = reinterpret_cast<LfnEntry*>(&entries[writeIdx]);
            memset(lfn, 0xFF, sizeof(LfnEntry));

            lfn->order = static_cast<u8>(seq);
            if (seq == lfnCount) lfn->order |= LFN_LAST_ENTRY;
            lfn->attr = DirAttr::LfnMask;
            lfn->type = 0;
            lfn->checksum = checksum;
            lfn->firstClusterLow = 0;

            u32 charBase = (seq - 1) * LFN_CHARS_PER_ENTRY;
            u32 charIdx = 0;

            for (u32 j = 0; j < 5; j++) {
                u32 pos = charBase + charIdx++;
                lfn->name1[j] = (pos < nameLen) ? static_cast<u16>(name[pos]) : (pos == nameLen ? 0x0000 : 0xFFFF);
            }
            for (u32 j = 0; j < 6; j++) {
                u32 pos = charBase + charIdx++;
                lfn->name2[j] = (pos < nameLen) ? static_cast<u16>(name[pos]) : (pos == nameLen ? 0x0000 : 0xFFFF);
            }
            for (u32 j = 0; j < 2; j++) {
                u32 pos = charBase + charIdx++;
                lfn->name3[j] = (pos < nameLen) ? static_cast<u16>(name[pos]) : (pos == nameLen ? 0x0000 : 0xFFFF);
            }

            writeIdx++;
            if (writeIdx >= entriesPerCluster) {
                writeCluster(writeCluster_, clusterBuf);
                writeCluster_ = fatGet(writeCluster_);
                readCluster(writeCluster_, clusterBuf);
                entries = reinterpret_cast<DirEntry*>(clusterBuf);
                writeIdx = 0;
            }
        }
    }

    // Write the short entry.
    DirEntry* shortEntry = &entries[writeIdx];
    memset(shortEntry, 0, sizeof(DirEntry));
    memcpy(shortEntry->name, shortName, 11);
    shortEntry->attr = attr;

    if (attr & DirAttr::Directory) {
        // Allocate a cluster for the new directory's contents.
        u32 contentCluster = allocateCluster();
        if (contentCluster == 0) {
            UserHeap::free(clusterBuf);
            return false;
        }
        shortEntry->firstClusterHigh = static_cast<u16>(contentCluster >> 16);
        shortEntry->firstClusterLow = static_cast<u16>(contentCluster);

        // Initialize the directory with . and .. entries.
        u8* dirBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
        if (!dirBuf) {
            UserHeap::free(clusterBuf);
            return false;
        }
        memset(dirBuf, 0, bytesPerCluster);

        DirEntry* dotEntries = reinterpret_cast<DirEntry*>(dirBuf);

        // "." entry.
        memset(dotEntries[0].name, ' ', 11);
        dotEntries[0].name[0] = '.';
        dotEntries[0].attr = DirAttr::Directory;
        dotEntries[0].firstClusterHigh = static_cast<u16>(contentCluster >> 16);
        dotEntries[0].firstClusterLow = static_cast<u16>(contentCluster);

        // ".." entry.
        memset(dotEntries[1].name, ' ', 11);
        dotEntries[1].name[0] = '.';
        dotEntries[1].name[1] = '.';
        dotEntries[1].attr = DirAttr::Directory;
        u32 parentCluster = (dirCluster == rootCluster) ? 0 : dirCluster;
        dotEntries[1].firstClusterHigh = static_cast<u16>(parentCluster >> 16);
        dotEntries[1].firstClusterLow = static_cast<u16>(parentCluster);

        writeCluster(contentCluster, dirBuf);
        UserHeap::free(dirBuf);
        flushFat();
    }

    *outCluster = writeCluster_;
    *outOffset = writeIdx * sizeof(DirEntry);

    writeCluster(writeCluster_, clusterBuf);
    UserHeap::free(clusterBuf);
    return true;
}

bool Fat32Filesystem::removeEntry(u32 dirCluster, const char* name) {
    u8* clusterBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
    if (!clusterBuf) return false;

    u32 cluster = dirCluster;
    while (cluster >= 2 && cluster < FAT_EOC) {
        if (!readCluster(cluster, clusterBuf)) {
            UserHeap::free(clusterBuf);
            return false;
        }

        u32 entriesPerCluster = bytesPerCluster / sizeof(DirEntry);
        DirEntry* entries = reinterpret_cast<DirEntry*>(clusterBuf);
        u32 lfnStart = 0;
        bool inLfn = false;

        for (u32 i = 0; i < entriesPerCluster; i++) {
            if (entries[i].name[0] == DIR_ENTRY_END) {
                UserHeap::free(clusterBuf);
                return false;
            }
            if (entries[i].name[0] == DIR_ENTRY_FREE) {
                inLfn = false;
                continue;
            }

            if (isLfnEntry(&entries[i])) {
                if (!inLfn) {
                    lfnStart = i;
                    inLfn = true;
                }
                continue;
            }

            if (entries[i].attr & DirAttr::VolumeId) {
                inLfn = false;
                continue;
            }

            // Check name match.
            char entryName[MAX_NAME];
            if (inLfn) {
                extractLfn(entries, lfnStart, i, entryName, MAX_NAME);
            } else {
                shortNameToString(entries[i].name, entryName, MAX_NAME);
            }

            if (nameEquals(entryName, name)) {
                // Check if directory is non-empty.
                if (entries[i].attr & DirAttr::Directory) {
                    u32 contentCluster =
                        (static_cast<u32>(entries[i].firstClusterHigh) << 16) |
                         static_cast<u32>(entries[i].firstClusterLow);

                    // Check for any real entries (skip . and ..).
                    char tmpName[MAX_NAME];
                    DirEntry tmpEntry;
                    u32 tc, to;
                    if (readDirEntry(contentCluster, 0, tmpName, MAX_NAME,
                                     &tmpEntry, &tc, &to)) {
                        UserHeap::free(clusterBuf);
                        return false;  // Directory not empty.
                    }

                    freeChain(contentCluster);
                }

                // Free the file's data clusters.
                u32 fileCluster =
                    (static_cast<u32>(entries[i].firstClusterHigh) << 16) |
                     static_cast<u32>(entries[i].firstClusterLow);
                if (!(entries[i].attr & DirAttr::Directory) && fileCluster >= 2) {
                    freeChain(fileCluster);
                }

                // Mark the short entry and all preceding LFN entries as deleted.
                entries[i].name[0] = DIR_ENTRY_FREE;
                if (inLfn) {
                    for (u32 j = lfnStart; j < i; j++) {
                        entries[j].name[0] = DIR_ENTRY_FREE;
                    }
                }

                writeCluster(cluster, clusterBuf);
                flushFat();
                UserHeap::free(clusterBuf);
                return true;
            }

            inLfn = false;
        }

        cluster = fatGet(cluster);
    }

    UserHeap::free(clusterBuf);
    return false;
}

// ---------------------------------------------------------------------------
// Mount
// ---------------------------------------------------------------------------

bool Fat32Filesystem::mount(u32 ataServicePid) {
    ataPid = ataServicePid;

    // Initialize handles.
    for (u32 i = 0; i < MAX_HANDLES; i++) {
        handles[i].inUse = false;
    }

    // Initialize sector cache.
    cacheAge = 0;
    for (u32 i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].data = nullptr;
        cache[i].lba = 0;
        cache[i].age = 0;
    }

    // Read the boot sector.
    u8 bootSector[SECTOR_SIZE];
    if (!readSector(0, bootSector)) return false;

    const Bpb* bpb = reinterpret_cast<const Bpb*>(bootSector);

    bytesPerSector = bpb->bytesPerSector;
    sectorsPerCluster = bpb->sectorsPerCluster;
    bytesPerCluster = bytesPerSector * sectorsPerCluster;
    reservedSectors = bpb->reservedSectors;
    numFats = bpb->numFats;
    fatSize = bpb->fatSize32;
    rootCluster = bpb->rootCluster;

    fatStartSector = reservedSectors;
    dataStartSector = reservedSectors + numFats * fatSize;

    u32 totalSectors = bpb->totalSectors32;
    u32 dataSectors = totalSectors - dataStartSector;
    totalClusters = dataSectors / sectorsPerCluster;
    fatEntryCount = totalClusters + 2;

    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool Fat32Filesystem::createDirectory(const char* path) {
    char name[MAX_NAME];
    u32 parentCluster = resolveParentPath(path, name, MAX_NAME);
    if (parentCluster == 0 || name[0] == '\0') return false;

    // Check if it already exists.
    DirEntry existing;
    u32 ec, eo;
    if (findEntry(parentCluster, name, &existing, &ec, &eo)) return false;

    u32 outCluster, outOffset;
    return createEntry(parentCluster, name, DirAttr::Directory,
                       &outCluster, &outOffset);
}

bool Fat32Filesystem::remove(const char* path) {
    if (!path || streq(path, "/")) return false;

    char name[MAX_NAME];
    u32 parentCluster = resolveParentPath(path, name, MAX_NAME);
    if (parentCluster == 0 || name[0] == '\0') return false;

    return removeEntry(parentCluster, name);
}

u32 Fat32Filesystem::open(const char* path, bool create) {
    DirEntry entry;
    u32 ec, eo;
    u32 cluster = resolvePath(path, &entry, &ec, &eo);

    if (cluster == 0) {
        if (!create) return 0;

        // Create the file.
        char name[MAX_NAME];
        u32 parentCluster = resolveParentPath(path, name, MAX_NAME);
        if (parentCluster == 0 || name[0] == '\0') return 0;

        if (!createEntry(parentCluster, name, DirAttr::Archive, &ec, &eo)) {
            return 0;
        }

        // Read back the created entry.
        u8* sectorBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
        if (!sectorBuf) return 0;
        readCluster(ec, sectorBuf);
        DirEntry* entries = reinterpret_cast<DirEntry*>(sectorBuf);
        u32 idx = eo / sizeof(DirEntry);
        memcpy(&entry, &entries[idx], sizeof(DirEntry));
        UserHeap::free(sectorBuf);

        cluster = (static_cast<u32>(entry.firstClusterHigh) << 16) |
                   static_cast<u32>(entry.firstClusterLow);
    }

    // Don't open directories.
    if (entry.attr & DirAttr::Directory) return 0;

    // Find a free handle slot.
    for (u32 i = 0; i < MAX_HANDLES; i++) {
        if (!handles[i].inUse) {
            handles[i].startCluster = (static_cast<u32>(entry.firstClusterHigh) << 16) |
                                       static_cast<u32>(entry.firstClusterLow);
            handles[i].size = entry.fileSize;
            handles[i].dirCluster = ec;
            handles[i].dirOffset = eo;
            handles[i].isDir = false;
            handles[i].inUse = true;
            return i + 1;
        }
    }

    return 0;
}

i32 Fat32Filesystem::read(u32 handle, u32 offset, u8* buf, u32 len) {
    if (handle == 0 || handle > MAX_HANDLES) return -1;
    FileHandle& fh = handles[handle - 1];
    if (!fh.inUse) return -1;
    if (offset >= fh.size) return 0;

    u32 available = fh.size - offset;
    u32 toRead = (len < available) ? len : available;

    u8* clusterBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
    if (!clusterBuf) return -1;

    u32 bytesRead = 0;
    while (bytesRead < toRead) {
        u32 filePos = offset + bytesRead;
        u32 cluster = clusterAtOffset(fh.startCluster, filePos);
        if (cluster == 0 || cluster >= FAT_EOC) break;

        if (!readCluster(cluster, clusterBuf)) {
            UserHeap::free(clusterBuf);
            return -1;
        }

        u32 clusterOffset = filePos % bytesPerCluster;
        u32 canRead = bytesPerCluster - clusterOffset;
        u32 remaining = toRead - bytesRead;
        u32 chunk = (canRead < remaining) ? canRead : remaining;

        memcpy(buf + bytesRead, clusterBuf + clusterOffset, chunk);
        bytesRead += chunk;
    }

    UserHeap::free(clusterBuf);
    return static_cast<i32>(bytesRead);
}

bool Fat32Filesystem::write(u32 handle, const u8* data, u32 len) {
    if (handle == 0 || handle > MAX_HANDLES) return false;
    FileHandle& fh = handles[handle - 1];
    if (!fh.inUse) return false;

    u8* clusterBuf = static_cast<u8*>(UserHeap::alloc(bytesPerCluster));
    if (!clusterBuf) return false;

    // If the file has no clusters yet, allocate the first one.
    if (fh.startCluster < 2 && len > 0) {
        u32 newCluster = allocateCluster();
        if (newCluster == 0) {
            UserHeap::free(clusterBuf);
            return false;
        }
        fh.startCluster = newCluster;
    }

    // Write data cluster by cluster.
    u32 bytesWritten = 0;
    u32 cluster = fh.startCluster;
    u32 prevCluster = 0;

    while (bytesWritten < len) {
        if (cluster < 2 || cluster >= FAT_EOC) {
            // Need another cluster.
            u32 newCluster = allocateCluster();
            if (newCluster == 0) {
                UserHeap::free(clusterBuf);
                return false;
            }
            if (prevCluster >= 2) {
                fatSet(prevCluster, newCluster);
            }
            cluster = newCluster;
        }

        u32 clusterOffset = bytesWritten % bytesPerCluster;
        u32 canWrite = bytesPerCluster - clusterOffset;
        u32 remaining = len - bytesWritten;
        u32 chunk = (canWrite < remaining) ? canWrite : remaining;

        // If partial cluster write, read existing data first.
        if (clusterOffset > 0 || chunk < bytesPerCluster) {
            readCluster(cluster, clusterBuf);
        }

        memcpy(clusterBuf + clusterOffset, data + bytesWritten, chunk);
        writeCluster(cluster, clusterBuf);

        bytesWritten += chunk;
        if (bytesWritten < len) {
            prevCluster = cluster;
            cluster = fatGet(cluster);
        }
    }

    // Free any leftover clusters if the new data is shorter.
    if (cluster >= 2 && cluster < FAT_EOC) {
        u32 next = fatGet(cluster);
        fatSet(cluster, FAT_EOC);
        if (next >= 2 && next < FAT_EOC) {
            freeChain(next);
        }
    }

    // Update the file size and start cluster in the directory entry.
    fh.size = len;

    if (!readCluster(fh.dirCluster, clusterBuf)) {
        UserHeap::free(clusterBuf);
        return false;
    }

    DirEntry* entries = reinterpret_cast<DirEntry*>(clusterBuf);
    u32 idx = fh.dirOffset / sizeof(DirEntry);
    entries[idx].fileSize = len;
    entries[idx].firstClusterHigh = static_cast<u16>(fh.startCluster >> 16);
    entries[idx].firstClusterLow = static_cast<u16>(fh.startCluster);

    writeCluster(fh.dirCluster, clusterBuf);
    flushFat();

    UserHeap::free(clusterBuf);
    return true;
}

u32 Fat32Filesystem::stat(const char* path) {
    if (!path || path[0] == '\0') return 0;
    if (streq(path, "/")) return 2;

    DirEntry entry;
    u32 cluster = resolvePath(path, &entry, nullptr, nullptr);
    if (cluster == 0) return 0;
    return (entry.attr & DirAttr::Directory) ? 2 : 1;
}

bool Fat32Filesystem::listEntry(const char* path, u32 index, char* nameOut, u32 nameMax) {
    DirEntry dirEntry;
    u32 dirCluster = resolvePath(path, &dirEntry, nullptr, nullptr);
    if (dirCluster == 0) return false;
    if (!(dirEntry.attr & DirAttr::Directory)) return false;

    DirEntry entry;
    u32 ec, eo;
    return readDirEntry(dirCluster, index, nameOut, nameMax, &entry, &ec, &eo);
}
