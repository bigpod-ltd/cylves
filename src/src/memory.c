/**
 * @file memory.c
 * @brief Memory management implementation
 */

#include "sylves/memory.h"
#include <string.h>

/* Default allocator functions */
static void* default_alloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void default_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

static void* default_realloc(void* ptr, size_t new_size, void* user_data) {
    (void)user_data;
    return realloc(ptr, new_size);
}

/* Default allocator instance */
static SylvesAllocator default_allocator = {
    .alloc = default_alloc,
    .free = default_free,
    .realloc = default_realloc,
    .user_data = NULL
};

/* Current allocator (defaults to standard malloc/free) */
static SylvesAllocator* current_allocator = &default_allocator;

SylvesAllocator* sylves_get_default_allocator(void) {
    return &default_allocator;
}

void sylves_set_allocator(const SylvesAllocator* allocator) {
    if (allocator) {
        current_allocator = (SylvesAllocator*)allocator;
    } else {
        current_allocator = &default_allocator;
    }
}

void* sylves_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    if (current_allocator && current_allocator->alloc) {
        return current_allocator->alloc(size, current_allocator->user_data);
    }
    
    return malloc(size);
}

void sylves_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    if (current_allocator && current_allocator->free) {
        current_allocator->free(ptr, current_allocator->user_data);
    } else {
        free(ptr);
    }
}

void* sylves_realloc(void* ptr, size_t new_size) {
    if (new_size == 0) {
        sylves_free(ptr);
        return NULL;
    }
    
    if (current_allocator && current_allocator->realloc) {
        return current_allocator->realloc(ptr, new_size, current_allocator->user_data);
    }
    
    return realloc(ptr, new_size);
}

void* sylves_calloc(size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = sylves_alloc(total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

void* sylves_memdup(const void* src, size_t size) {
    if (!src || size == 0) {
        return NULL;
    }
    
    void* dst = sylves_alloc(size);
    if (dst) {
        memcpy(dst, src, size);
    }
    
    return dst;
}
