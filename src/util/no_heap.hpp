// src/no_heap.hpp
#pragma once

#include <cstddef>
#include <new>

#if defined(__clang__)
#  define NO_HEAP_ERROR(msg) __attribute__((diagnose_if(true, msg, "error")))
#elif defined(__GNUC__)
#  define NO_HEAP_ERROR(msg) __attribute__((error(msg)))
#else
#  define NO_HEAP_ERROR(msg)
#endif

// #include <cstddef>
//
// extern "C" {
//
// #if defined(__clang__)
//     __attribute__((diagnose_if(true, "malloc forbidden", "error")))
//     void* malloc(std::size_t);
//     __attribute__((diagnose_if(true, "calloc forbidden", "error")))
//     void* calloc(std::size_t, std::size_t);
//     __attribute__((diagnose_if(true, "realloc forbidden", "error")))
//     void* realloc(void*, std::size_t);
//     __attribute__((diagnose_if(true, "free forbidden", "error")))
//     void  free(void*);
// #elif defined(__GNUC__)
//     __attribute__((error("malloc forbidden")))
//     void* malloc(std::size_t);
//     __attribute__((error("calloc forbidden")))
//     void* calloc(std::size_t, std::size_t);
//     __attribute__((error("realloc forbidden")))
//     void* realloc(void*, std::size_t);
//     __attribute__((error("free forbidden")))
//     void  free(void*);
// #endif
//
// }

NO_HEAP_ERROR("heap allocation forbidden: operator new")
void* operator new(std::size_t);

NO_HEAP_ERROR("heap allocation forbidden: operator new[]")
void* operator new[](std::size_t);

NO_HEAP_ERROR("heap allocation forbidden: nothrow operator new")
void* operator new(std::size_t, const std::nothrow_t&) noexcept;

NO_HEAP_ERROR("heap allocation forbidden: nothrow operator new[]")
void* operator new[](std::size_t, const std::nothrow_t&) noexcept;

#if defined(__cpp_aligned_new)
NO_HEAP_ERROR("heap allocation forbidden: aligned operator new")
void* operator new(std::size_t, std::align_val_t);

NO_HEAP_ERROR("heap allocation forbidden: aligned operator new[]")
void* operator new[](std::size_t, std::align_val_t);

NO_HEAP_ERROR("heap allocation forbidden: aligned nothrow operator new")
void* operator new(std::size_t, std::align_val_t, const std::nothrow_t&) noexcept;

NO_HEAP_ERROR("heap allocation forbidden: aligned nothrow operator new[]")
void* operator new[](std::size_t, std::align_val_t, const std::nothrow_t&) noexcept;
#endif

NO_HEAP_ERROR("delete forbidden")
void operator delete(void*) noexcept;

NO_HEAP_ERROR("delete[] forbidden")
void operator delete[](void*) noexcept;

NO_HEAP_ERROR("sized delete forbidden")
void operator delete(void*, std::size_t) noexcept;

NO_HEAP_ERROR("sized delete[] forbidden")
void operator delete[](void*, std::size_t) noexcept;

#if defined(__cpp_aligned_new)
NO_HEAP_ERROR("aligned delete forbidden")
void operator delete(void*, std::align_val_t) noexcept;

NO_HEAP_ERROR("aligned delete[] forbidden")
void operator delete[](void*, std::align_val_t) noexcept;

NO_HEAP_ERROR("sized aligned delete forbidden")
void operator delete(void*, std::size_t, std::align_val_t) noexcept;

NO_HEAP_ERROR("sized aligned delete[] forbidden")
void operator delete[](void*, std::size_t, std::align_val_t) noexcept;
#endif

#undef NO_HEAP_ERROR
