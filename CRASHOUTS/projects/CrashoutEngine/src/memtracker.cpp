/*!
@file       memtracker.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       06/09/2025

This file provides the Crashout Engine's ImGUI to track allocation, deallocation, and execution timing.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "../include/performanceViewer.h" 
#include <cstdlib>
#include <new>
//---------------------------------------------------------------------------
using namespace std;

/*!
    @brief  wraps malloc with metadata
    @return the pointer to the user memory
*/
static inline char* alloc_block(size_t sz, uint8_t tag) {
    const size_t total = sz + sizeof(size_t) + 1;
    char* p = (char*)std::malloc(total);
    if (!p) throw std::bad_alloc();
    p[0] = (char)tag;
    *reinterpret_cast<size_t*>(p + 1) = sz;

    auto& tr = MemTracker::instance();
    tr.live[tag] += (long long)sz;
    tr.allocs[tag] += (long long)sz;
    return p + sizeof(size_t) + 1;
}

/*!
    @brief  redirects all allocation to alloc_block()
*/
void* operator new(size_t sz) {
    return alloc_block(sz, (uint8_t)g_memTagTLS);
}
void* operator new[](size_t sz) {
    return alloc_block(sz, (uint8_t)g_memTagTLS);
}

/*!
    @brief  retrieves the metadata from header, updates live and frees
    @return calls for free
*/
static inline void free_block(void* vp) noexcept {
    if (!vp) return;
    char* q = (char*)vp - (sizeof(size_t) + 1);
    auto tag = (size_t)q[0];
    size_t sz = *reinterpret_cast<size_t*>(q + 1);

    auto& tr = MemTracker::instance();
    tr.live[tag] -= (long long)sz;
    tr.frees[tag] += (long long)sz;
    std::free(q);
}

void operator delete(void* p) noexcept { free_block(p); }
void operator delete[](void* p) noexcept { free_block(p); }
void operator delete(void* p, size_t) noexcept { free_block(p); }
void operator delete[](void* p, size_t) noexcept { free_block(p); }

void* operator new(size_t sz, const std::nothrow_t&) noexcept(false) {
    return alloc_block(sz, (uint8_t)g_memTagTLS);
}
void* operator new[](size_t sz, const std::nothrow_t&) noexcept(false) {
    return alloc_block(sz, (uint8_t)g_memTagTLS);
}
void operator delete(void* p, const std::nothrow_t&) noexcept { free_block(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { free_block(p); }