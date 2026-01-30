/*!
@file		memorypool.cpp
@author     m.biniswandi@digipen.edu
@date       06/01/2026
@brief		im getting cooked here chat

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "memorypool.h"

bool MemMgrConfig::s_useMemoryPool = true;

static std::size_t AlignUp(std::size_t v, std::size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

void MemoryPool::Init(std::size_t blockSize, std::size_t blocksPerPage, std::size_t typeAlignment) {
    // Ensure block can store a pointer for freelist
    // We need to store FreeNode in the block when it's free.
    // So the block size must be at least sizeof(FreeNode).
    // AND the alignment of the block must satisfy BOTH T and FreeNode.
    
    std::size_t requiredAlignment = std::max(typeAlignment, alignof(FreeNode));

    m_blockSize = (blockSize < sizeof(FreeNode)) ? sizeof(FreeNode) : blockSize;
    m_blockSize = AlignUp(m_blockSize, requiredAlignment);
    m_blocksPerPage = blocksPerPage;

    AllocateNewPage();
}

void MemoryPool::Shutdown() {
    // free all pages
    PageHeader* p = m_pages;
    while (p) {
        PageHeader* next = p->next;
        ::operator delete(p);
        p = next;
    }
    m_pages = nullptr;
    m_freeList = nullptr;
    m_pagesAllocated = 0;
    m_activeAllocs = 0;
}

void MemoryPool::AllocateNewPage() {
    const std::size_t headerSize = AlignUp(sizeof(PageHeader), alignof(std::max_align_t));
    const std::size_t pageBytes = headerSize + (m_blockSize * m_blocksPerPage);

    void* raw = ::operator new(pageBytes); // allowed: ONLY when extending pages
    auto* header = reinterpret_cast<PageHeader*>(raw);

    header->next = m_pages;
    m_pages = header;
    ++m_pagesAllocated;

    // Build freelist from blocks inside this page
    std::uint8_t* blocks = reinterpret_cast<std::uint8_t*>(raw) + headerSize;
    for (std::size_t i = 0; i < m_blocksPerPage; ++i) {
        auto* node = reinterpret_cast<FreeNode*>(blocks + i * m_blockSize);
        node->next = m_freeList;
        m_freeList = node;
    }
}

void* MemoryPool::Allocate() {
    if (!m_freeList) {
        AllocateNewPage(); // extend at runtime when out of preallocated
    }
    FreeNode* node = m_freeList;
    m_freeList = node->next;
    ++m_activeAllocs;
    return node;
}

void MemoryPool::Free(void* p) {
    if (!p) return;
    auto* node = reinterpret_cast<FreeNode*>(p);
    node->next = m_freeList;
    m_freeList = node;
    --m_activeAllocs;
}
