/**
 * @file memory.h
 * @brief Memory management utilities and patterns
 */

#ifndef SYLVES_MEMORY_H
#define SYLVES_MEMORY_H

#include <stddef.h>
#include <stdlib.h>
#include "errors.h"


/**
 * @brief Memory allocation function type
 */
typedef void* (*SylvesAllocFunc)(size_t size, void* user_data);

/**
 * @brief Memory deallocation function type
 */
typedef void (*SylvesFreeFunc)(void* ptr, void* user_data);

/**
 * @brief Memory reallocation function type
 */
typedef void* (*SylvesReallocFunc)(void* ptr, size_t new_size, void* user_data);

/**
 * @brief Memory allocator structure
 */
typedef struct {
    SylvesAllocFunc alloc;
    SylvesFreeFunc free;
    SylvesReallocFunc realloc;
    void* user_data;
} SylvesAllocator;

/**
 * @brief Get the default allocator (uses stdlib malloc/free)
 */
SylvesAllocator* sylves_get_default_allocator(void);

/**
 * @brief Set a custom allocator for the library
 */
void sylves_set_allocator(const SylvesAllocator* allocator);

/**
 * @brief Allocate memory using the current allocator
 */
void* sylves_alloc(size_t size);

/**
 * @brief Free memory using the current allocator
 */
void sylves_free(void* ptr);

/**
 * @brief Reallocate memory using the current allocator
 */
void* sylves_realloc(void* ptr, size_t new_size);

/**
 * @brief Allocate zeroed memory
 */
void* sylves_calloc(size_t count, size_t size);

/**
 * @brief Duplicate a memory block
 */
void* sylves_memdup(const void* src, size_t size);

/**
 * @brief Helper macros for type-safe allocation
 */
#define SYLVES_NEW(type) ((type*)sylves_alloc(sizeof(type)))
#define SYLVES_NEW_ARRAY(type, count) ((type*)sylves_alloc(sizeof(type) * (count)))
#define SYLVES_DELETE(ptr) sylves_free(ptr)
#define SYLVES_DELETE_ARRAY(ptr) sylves_free(ptr)

/**
 * @brief Reference counting structure
 */
typedef struct {
    int count;
} SylvesRefCount;

/**
 * @brief Initialize a reference count
 */
static inline void sylves_ref_init(SylvesRefCount* ref) {
    ref->count = 1;
}

/**
 * @brief Increment reference count
 */
static inline void sylves_ref_inc(SylvesRefCount* ref) {
    ref->count++;
}

/**
 * @brief Decrement reference count and return true if it reaches zero
 */
static inline bool sylves_ref_dec(SylvesRefCount* ref) {
    return --ref->count == 0;
}

/**
 * @brief Get current reference count
 */
static inline int sylves_ref_get(const SylvesRefCount* ref) {
    return ref->count;
}


#endif /* SYLVES_MEMORY_H */
