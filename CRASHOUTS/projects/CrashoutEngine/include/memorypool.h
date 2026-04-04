/*!
@file		memorypool.h
@author     Shahrin Idzuan
@date       06/01/2026
@brief		Memory pool header file

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <cstddef>
#include <cstdint>
#include <new>

/** @brief MemMgrConfig */
struct MemMgrConfig {
    static bool s_useMemoryPool;
};

/** @brief MemoryPool */
class MemoryPool {
public:
    MemoryPool() = default;
    ~MemoryPool() = default;

    void Init(std::size_t blockSize, std::size_t blocksPerPage, std::size_t typeAlignment);
    void Shutdown();

    void* Allocate();
    void  Free(void* p);

    // Debug / proof
    std::size_t GetPagesAllocated() const { return m_pagesAllocated; }
    std::size_t GetActiveAllocs()  const { return m_activeAllocs; }

private:
    /** @brief FreeNode */
    struct FreeNode { FreeNode* next; };

    /** @brief PageHeader */
    struct PageHeader {
        PageHeader* next;
        // blocks follow right after header
    };

    void AllocateNewPage();

private:
    std::size_t m_blockSize = 0;
    std::size_t m_blocksPerPage = 0;

    FreeNode* m_freeList = nullptr;
    PageHeader* m_pages = nullptr;

    std::size_t m_pagesAllocated = 0;
    std::size_t m_activeAllocs = 0;
};