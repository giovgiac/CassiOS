#include <std/test.hpp>

#include <memory/physical.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::memory;

TEST(pmm_has_free_frames_after_init) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    ASSERT(pmm.getTotalFrames() > 0);
}

TEST(pmm_alloc_returns_non_null) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    pmm.freeFrame(frame);
}

TEST(pmm_alloc_returns_aligned_address) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    ASSERT_EQ((u32)frame % FRAME_SIZE, 0u);
    pmm.freeFrame(frame);
}

TEST(pmm_alloc_marks_frame_used) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    ASSERT(pmm.isFrameUsed(frame));
    pmm.freeFrame(frame);
}

TEST(pmm_free_marks_frame_unused) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    pmm.freeFrame(frame);
    ASSERT(!pmm.isFrameUsed(frame));
}

TEST(pmm_alloc_returns_different_frames) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame1 = pmm.allocFrame();
    void* frame2 = pmm.allocFrame();
    ASSERT(frame1 != nullptr);
    ASSERT(frame2 != nullptr);
    ASSERT(frame1 != frame2);
    pmm.freeFrame(frame2);
    pmm.freeFrame(frame1);
}

TEST(pmm_free_allows_realloc) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame1 = pmm.allocFrame();
    ASSERT(frame1 != nullptr);
    pmm.freeFrame(frame1);
    void* frame2 = pmm.allocFrame();
    ASSERT(frame2 != nullptr);
    ASSERT_EQ((u32)frame1, (u32)frame2);
    pmm.freeFrame(frame2);
}

TEST(pmm_kernel_region_is_used) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    // Kernel is loaded at 1 MiB (0x100000)
    ASSERT(pmm.isFrameUsed((void*)0x100000));
}

TEST(pmm_null_page_is_used) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    // Frame at address 0 should always be marked as used
    ASSERT(pmm.isFrameUsed((void*)0x0));
}

TEST(pmm_free_frames_decreases_on_alloc) {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    u32 before = pmm.getFreeFrames();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    u32 after = pmm.getFreeFrames();
    ASSERT_EQ(before - 1, after);
    pmm.freeFrame(frame);
}
