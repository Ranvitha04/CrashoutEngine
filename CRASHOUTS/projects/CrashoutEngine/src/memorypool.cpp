/*!
@file		memorypool.cpp
@author     m.biniswandi@digipen.edu
@date       06/01/2026
@brief		im getting cooked here chat

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "memorypool.h"

#include <cstdlib>   // malloc, free
#include <cstring>   // memset
#include <cassert>

// Constructor
MemoryPool::MemoryPool()
    : m_BlockSize(0),
    m_BlocksPerPage(0),
    m_FreeList(nullptr),
    m_Pages(nullptr)
{
}

// Initialize the memory pool
void MemoryPool::Init(size_t blockSize, size_t blocksPerPage)
{
    assert(blockSize >= sizeof(FreeNode) && "Block size too small");
    assert(blocksPerPage > 0);

    m_BlockSize = blockSize;
    m_BlocksPerPage = blocksPerPage;

    m_FreeList = nullptr;
    m_Pages = nullptr;

    // Allocate the first page immediately
    AllocateNewPage();
}

// Allocate a block from the pool
void* MemoryPool::Allocate()
{
    // If no free blocks, grow pool dynamically
    if (!m_FreeList)
    {
        AllocateNewPage();
    }

    // Pop from free list
    FreeNode* node = m_FreeList;
    m_FreeList = node->Next;

    return node;
}

// Free a block back to the pool
void MemoryPool::Free(void* ptr)
{
    if (!ptr)
        return;

    // Push back into free list
    FreeNode* node = static_cast<FreeNode*>(ptr);
    node->Next = m_FreeList;
    m_FreeList = node;
}

// Allocate a new page and populate free list
void MemoryPool::AllocateNewPage()
{
    // Allocate raw memory for the page
    size_t pageSize = m_BlockSize * m_BlocksPerPage;
    void* pageMemory = std::malloc(pageSize);
    assert(pageMemory && "MemoryPool page allocation failed");

    // clear memory for safety
    std::memset(pageMemory, 0, pageSize);

    // Track the page
    Page* newPage = static_cast<Page*>(std::malloc(sizeof(Page)));
    assert(newPage && "MemoryPool page header allocation failed");

    newPage->Memory = pageMemory;
    newPage->Next = m_Pages;
    m_Pages = newPage;

    // Split page into blocks and push into free list
    char* blockPtr = static_cast<char*>(pageMemory);

    for (size_t i = 0; i < m_BlocksPerPage; ++i)
    {
        FreeNode* node = reinterpret_cast<FreeNode*>(blockPtr);
        node->Next = m_FreeList;
        m_FreeList = node;

        blockPtr += m_BlockSize;
    }
}
