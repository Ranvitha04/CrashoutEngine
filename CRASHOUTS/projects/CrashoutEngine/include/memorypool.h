/*!
@file		memorypool.h
@author     m.biniswandi@digipen.edu
@date       06/01/2026
@brief		im getting cooked here chat

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <cstddef>   // for size_t

class MemoryPool
{
public:
    MemoryPool();

    void Init(size_t blockSize, size_t blocksPerPage);
    void* Allocate();
    void Free(void* ptr);

private:
    struct FreeNode
	{
    	FreeNode* Next;
    };

    struct Page
	{
    	Page* Next;
        void* Memory;
    };

    // Pool configuration
    size_t m_BlockSize;
    size_t m_BlocksPerPage;

    // Pool state
    FreeNode* m_FreeList;
    Page* m_Pages;

private:
    void AllocateNewPage();  // internal helper
};
