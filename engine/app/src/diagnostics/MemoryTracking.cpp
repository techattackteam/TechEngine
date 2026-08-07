#include <TechEngine/base/diagnostics/Profile.hpp>

#include <diagnostics/MemoryTracking.hpp>

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace TechEngine {
    void memoryTrackingAnchor() {
    }
}

// ASan and TSan replace the global allocation functions themselves, so a second replacement
// in the same binary fails to link. Silently dropping the memory plot is the only option that
// leaves such a build usable at all; every other configuration keeps it.
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define TE_SANITIZER_OWNS_ALLOCATOR
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define TE_SANITIZER_OWNS_ALLOCATOR
#endif

#if defined(TE_PROFILE_ENABLED) && !defined(TE_SANITIZER_OWNS_ALLOCATOR)

static void* techEngineAllocate(std::size_t size) {
    if (size == 0) {
        size = 1;
    }
    while (true) {
        void* pointer = std::malloc(size);
        if (pointer != nullptr) {
            TE_PROFILER_ALLOC(pointer, size);
            return pointer;
        }
        // A replacement that skips the handler turns a recoverable OOM into a hard failure
        // for any allocator that installed one.
        const std::new_handler handler = std::get_new_handler();
        if (handler == nullptr) {
            return nullptr;
        }
        handler();
    }
}

static void* techEngineAllocateAligned(std::size_t size, std::align_val_t alignment) {
    const std::size_t bytes = static_cast<std::size_t>(alignment);
    if (size == 0) {
        size = bytes;
    }
    while (true) {
#if defined(_MSC_VER)
        void* pointer = _aligned_malloc(size, bytes);
#else
        // std::aligned_alloc requires the size to be a multiple of the alignment.
        const std::size_t rounded = ((size + bytes - 1) / bytes) * bytes;
        void* pointer = std::aligned_alloc(bytes, rounded);
#endif
        if (pointer != nullptr) {
            TE_PROFILER_ALLOC(pointer, size);
            return pointer;
        }
        const std::new_handler handler = std::get_new_handler();
        if (handler == nullptr) {
            return nullptr;
        }
        handler();
    }
}

static void* techEngineAllocateOrThrow(std::size_t size) {
    void* pointer = techEngineAllocate(size);
    if (pointer == nullptr) {
        throw std::bad_alloc{};
    }
    return pointer;
}

static void* techEngineAllocateAlignedOrThrow(std::size_t size, std::align_val_t alignment) {
    void* pointer = techEngineAllocateAligned(size, alignment);
    if (pointer == nullptr) {
        throw std::bad_alloc{};
    }
    return pointer;
}

static void techEngineDeallocate(void* pointer) noexcept {
    if (pointer == nullptr) {
        return;
    }
    TE_PROFILER_FREE(pointer);
    std::free(pointer);
}

static void techEngineDeallocateAligned(void* pointer) noexcept {
    if (pointer == nullptr) {
        return;
    }
    TE_PROFILER_FREE(pointer);
#if defined(_MSC_VER)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

void* operator new(std::size_t size) {
    return techEngineAllocateOrThrow(size);
}

void* operator new[](std::size_t size) {
    return techEngineAllocateOrThrow(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return techEngineAllocateAlignedOrThrow(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return techEngineAllocateAlignedOrThrow(size, alignment);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return techEngineAllocate(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return techEngineAllocate(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try {
        return techEngineAllocateAligned(size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try {
        return techEngineAllocateAligned(size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void* pointer) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete[](void* pointer) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete(void* pointer, const std::nothrow_t&) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) noexcept {
    techEngineDeallocate(pointer);
}

void operator delete(void* pointer, std::align_val_t) noexcept {
    techEngineDeallocateAligned(pointer);
}

void operator delete[](void* pointer, std::align_val_t) noexcept {
    techEngineDeallocateAligned(pointer);
}

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    techEngineDeallocateAligned(pointer);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    techEngineDeallocateAligned(pointer);
}

void operator delete(void* pointer, std::align_val_t, const std::nothrow_t&) noexcept {
    techEngineDeallocateAligned(pointer);
}

void operator delete[](void* pointer, std::align_val_t, const std::nothrow_t&) noexcept {
    techEngineDeallocateAligned(pointer);
}

#endif

#undef TE_SANITIZER_OWNS_ALLOCATOR
