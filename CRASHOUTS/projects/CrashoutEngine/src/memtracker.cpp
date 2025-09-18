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
// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/*!
@brief  Allocate a bookkeeping block and return the user pointer.
@param  sz   Requested user size in bytes.
@param  tag  Active tag at the time of allocation.
@returns Pointer to user block; throws std::bad_alloc on failure.
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
@brief  Free a block previously returned by @ref alloc_block.
@param  vp   User pointer.
@note   Safely handles null pointers.
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

/*!
@brief  Dispatch helper that tags allocations using thread-local @ref g_memTagTLS.
*/
static inline void* alloc_dispatch(size_t sz) {
    try { return alloc_block(sz, (uint8_t)g_memTagTLS); }
    catch (...) { return nullptr; }
}

// ---------------------------------------------------------------------------
// Global operator new/delete overrides
// ---------------------------------------------------------------------------

// normal new/delete
void* operator new(size_t sz) { return alloc_dispatch(sz); }
void* operator new[](size_t sz) { return alloc_dispatch(sz); }
void  operator delete(void* p) noexcept { free_block(p); }
void  operator delete[](void* p) noexcept { free_block(p); }

// sized delete (C++14)
void  operator delete(void* p, size_t) noexcept { free_block(p); }
void  operator delete[](void* p, size_t) noexcept { free_block(p); }

// nothrow new/delete
void* operator new(size_t sz, const std::nothrow_t&) noexcept { return alloc_dispatch(sz); }
void* operator new[](size_t sz, const std::nothrow_t&) noexcept { return alloc_dispatch(sz); }

// aligned new/delete
void* operator new(size_t sz, std::align_val_t) { return alloc_dispatch(sz); }
void* operator new[](size_t sz, std::align_val_t) { return alloc_dispatch(sz); }
void  operator delete(void* p, std::align_val_t) noexcept { free_block(p); }
void  operator delete[](void* p, std::align_val_t) noexcept { free_block(p); }
void  operator delete(void* p, size_t, std::align_val_t) noexcept { free_block(p); }
void  operator delete[](void* p, size_t, std::align_val_t) noexcept { free_block(p); }