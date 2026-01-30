/*!
@file		memorypool.h
@author     m.biniswandi@digipen.edu
@date       06/01/2026
@brief		im getting cooked here chat

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <cstddef>
#include <cstdint>
#include <new>

struct MemMgrConfig {
    static bool s_useMemoryPool;
};

class MemoryPool {
public:
    MemoryPool() = default;
    ~MemoryPool() = default;

    void Init(std::size_t blockSize, std::size_t blocksPerPage);
    void Shutdown();

    void* Allocate();
    void  Free(void* p);

    // Debug / proof
    std::size_t GetPagesAllocated() const { return m_pagesAllocated; }
    std::size_t GetActiveAllocs()  const { return m_activeAllocs; }

private:
    struct FreeNode { FreeNode* next; };

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