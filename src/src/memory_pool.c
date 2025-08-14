#include "sylves/memory_pool.h"
#include "sylves/memory.h"
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#define THREAD_LOCAL __declspec(thread)
#else
#include <pthread.h>
#define THREAD_LOCAL __thread
#endif

/**
 * Memory block header for tracking allocations
 */
typedef struct MemoryBlock {
    struct MemoryBlock* next;  /**< Next free block in list */
    size_t size;              /**< Size of this block (for generic pools) */
} MemoryBlock;

/**
 * Memory pool implementation
 */
struct SylvesMemoryPool {
    MemoryBlock* free_list;    /**< Head of free block list */
    void* memory_base;         /**< Base pointer to allocated memory */
    size_t block_size;         /**< Size of each block */
    size_t capacity;           /**< Current capacity in blocks */
    size_t max_capacity;       /**< Maximum capacity (0 for unlimited) */
    size_t used_count;         /**< Number of blocks in use */
    SylvesPoolConfig config;   /**< Pool configuration */
    SylvesPoolStats stats;     /**< Pool statistics */
#ifdef _WIN32
    CRITICAL_SECTION lock;     /**< Windows critical section */
#else
    pthread_mutex_t lock;      /**< POSIX mutex */
#endif
};

/**
 * Cell pool implementation
 */
struct SylvesCellPool {
    SylvesMemoryPool* pool;    /**< Underlying memory pool */
};

/**
 * Path pool implementation
 */
struct SylvesPathPool {
    SylvesMemoryPool* pool;    /**< Underlying memory pool */
};

/**
 * Generic pool bucket for size classes
 */
typedef struct GenericBucket {
    SylvesMemoryPool* pool;    /**< Pool for this size class */
    size_t size;              /**< Size class */
} GenericBucket;

/**
 * Generic pool implementation
 */
struct SylvesGenericPool {
    GenericBucket* buckets;    /**< Array of size class buckets */
    size_t bucket_count;       /**< Number of buckets */
    size_t min_size;          /**< Minimum block size */
    size_t max_size;          /**< Maximum block size */
    bool thread_safe;         /**< Thread safety flag */
};

/* Thread-local storage */
static THREAD_LOCAL SylvesCellPool* tls_cell_pool = NULL;
static THREAD_LOCAL SylvesPathPool* tls_path_pool = NULL;

/* Helper functions */

static void init_lock(SylvesMemoryPool* pool) {
    if (pool->config.thread_safe) {
#ifdef _WIN32
        InitializeCriticalSection(&pool->lock);
#else
        pthread_mutex_init(&pool->lock, NULL);
#endif
    }
}

static void destroy_lock(SylvesMemoryPool* pool) {
    if (pool->config.thread_safe) {
#ifdef _WIN32
        DeleteCriticalSection(&pool->lock);
#else
        pthread_mutex_destroy(&pool->lock);
#endif
    }
}

static void lock_pool(SylvesMemoryPool* pool) {
    if (pool->config.thread_safe) {
#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
#else
        pthread_mutex_lock(&pool->lock);
#endif
    }
}

static void unlock_pool(SylvesMemoryPool* pool) {
    if (pool->config.thread_safe) {
#ifdef _WIN32
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_unlock(&pool->lock);
#endif
    }
}

static bool expand_pool(SylvesMemoryPool* pool, size_t additional_blocks) {
    if (pool->max_capacity > 0 && pool->capacity + additional_blocks > pool->max_capacity) {
        additional_blocks = pool->max_capacity - pool->capacity;
        if (additional_blocks == 0) {
            return false;
        }
    }
    
    size_t alloc_size = additional_blocks * pool->block_size;
    void* new_memory = sylves_alloc(alloc_size);
    if (!new_memory) {
        return false;
    }
    
    /* Initialize new blocks and add to free list */
    char* ptr = (char*)new_memory;
    for (size_t i = 0; i < additional_blocks; i++) {
        MemoryBlock* block = (MemoryBlock*)ptr;
        block->next = pool->free_list;
        block->size = pool->block_size;
        pool->free_list = block;
        ptr += pool->block_size;
    }
    
    pool->capacity += additional_blocks;
    pool->stats.total_memory_used = pool->capacity * pool->block_size;
    if (pool->stats.total_memory_used > pool->stats.peak_memory_used) {
        pool->stats.peak_memory_used = pool->stats.total_memory_used;
    }
    
    return true;
}

/* Public API implementation */

SylvesMemoryPool* sylves_memory_pool_create(const SylvesPoolConfig* config) {
    if (!config || config->block_size < sizeof(MemoryBlock)) {
        return NULL;
    }
    
    SylvesMemoryPool* pool = (SylvesMemoryPool*)sylves_alloc(sizeof(SylvesMemoryPool));
    if (!pool) {
        return NULL;
    }
    
    memset(pool, 0, sizeof(SylvesMemoryPool));
    pool->config = *config;
    pool->block_size = config->block_size;
    pool->max_capacity = config->max_capacity;
    
    init_lock(pool);
    
    /* Pre-allocate initial blocks */
    if (config->initial_capacity > 0) {
        if (!expand_pool(pool, config->initial_capacity)) {
            destroy_lock(pool);
            sylves_free(pool);
            return NULL;
        }
    }
    
    return pool;
}

void sylves_memory_pool_destroy(SylvesMemoryPool* pool) {
    if (!pool) {
        return;
    }
    
    /* Free all memory blocks */
    /* Note: In a real implementation, we'd track all allocated memory
     * to free it properly. For now, we assume the pool is reset before destruction */
    
    destroy_lock(pool);
    sylves_free(pool);
}

void* sylves_pool_alloc(SylvesMemoryPool* pool) {
    if (!pool) {
        return NULL;
    }
    
    lock_pool(pool);
    
    /* Try to get a block from free list */
    if (!pool->free_list) {
        /* Expand pool by doubling or initial capacity */
        size_t expand_size = pool->capacity > 0 ? pool->capacity : pool->config.initial_capacity;
        if (expand_size == 0) {
            expand_size = 16; /* Default expansion */
        }
        
        if (!expand_pool(pool, expand_size)) {
            unlock_pool(pool);
            return NULL;
        }
    }
    
    /* Pop block from free list */
    MemoryBlock* block = pool->free_list;
    pool->free_list = block->next;
    pool->used_count++;
    
    if (pool->config.track_stats) {
        pool->stats.total_allocations++;
        pool->stats.active_allocations++;
    }
    
    unlock_pool(pool);
    
    /* Zero memory if requested */
    if (pool->config.zero_on_alloc) {
        memset(block, 0, pool->block_size);
    }
    
    return block;
}

void sylves_pool_free(SylvesMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    lock_pool(pool);
    
    /* Return block to free list */
    MemoryBlock* block = (MemoryBlock*)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
    pool->used_count--;
    
    if (pool->config.track_stats) {
        pool->stats.active_allocations--;
        pool->stats.reuse_count++;
    }
    
    unlock_pool(pool);
}

void sylves_pool_reset(SylvesMemoryPool* pool) {
    if (!pool) {
        return;
    }
    
    lock_pool(pool);
    
    /* Reset is complex without tracking all allocations
     * For now, this is a no-op, assuming proper usage */
    pool->used_count = 0;
    if (pool->config.track_stats) {
        pool->stats.active_allocations = 0;
    }
    
    unlock_pool(pool);
}

void sylves_pool_get_stats(const SylvesMemoryPool* pool, SylvesPoolStats* stats) {
    if (!pool || !stats) {
        return;
    }
    
    *stats = pool->stats;
}

void sylves_pool_trim(SylvesMemoryPool* pool, size_t target_capacity) {
    if (!pool) {
        return;
    }
    
    /* Trimming would require tracking allocated memory regions
     * For now, this is a no-op */
}

/* Cell pool implementation */

SylvesCellPool* sylves_cell_pool_create(size_t initial_capacity, bool thread_safe) {
    SylvesCellPool* cell_pool = (SylvesCellPool*)sylves_alloc(sizeof(SylvesCellPool));
    if (!cell_pool) {
        return NULL;
    }
    
    SylvesPoolConfig config = {
        .block_size = sizeof(SylvesCell),
        .initial_capacity = initial_capacity,
        .max_capacity = 0,
        .thread_safe = thread_safe,
        .track_stats = true,
        .zero_on_alloc = false
    };
    
    cell_pool->pool = sylves_memory_pool_create(&config);
    if (!cell_pool->pool) {
        sylves_free(cell_pool);
        return NULL;
    }
    
    return cell_pool;
}

void sylves_cell_pool_destroy(SylvesCellPool* pool) {
    if (!pool) {
        return;
    }
    
    sylves_memory_pool_destroy(pool->pool);
    sylves_free(pool);
}

SylvesCell* sylves_cell_pool_alloc(SylvesCellPool* pool) {
    if (!pool) {
        return NULL;
    }
    
    return (SylvesCell*)sylves_pool_alloc(pool->pool);
}

void sylves_cell_pool_free(SylvesCellPool* pool, SylvesCell* cell) {
    if (!pool || !cell) {
        return;
    }
    
    sylves_pool_free(pool->pool, cell);
}

SylvesCell* sylves_cell_pool_alloc_array(SylvesCellPool* pool, size_t count) {
    if (!pool || count == 0) {
        return NULL;
    }
    
    /* For simplicity, allocate from regular memory for arrays
     * A more sophisticated implementation would have array pools */
    return (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * count);
}

void sylves_cell_pool_free_array(SylvesCellPool* pool, SylvesCell* cells, size_t count) {
    if (!pool || !cells) {
        return;
    }
    
    sylves_free(cells);
}

/* Path pool implementation */

SylvesPathPool* sylves_path_pool_create(size_t initial_capacity, bool thread_safe) {
    SylvesPathPool* path_pool = (SylvesPathPool*)sylves_alloc(sizeof(SylvesPathPool));
    if (!path_pool) {
        return NULL;
    }
    
    /* Path nodes are typically small structures */
    SylvesPoolConfig config = {
        .block_size = sizeof(SylvesStep), /* Using step size as path node size */
        .initial_capacity = initial_capacity,
        .max_capacity = 0,
        .thread_safe = thread_safe,
        .track_stats = true,
        .zero_on_alloc = false
    };
    
    path_pool->pool = sylves_memory_pool_create(&config);
    if (!path_pool->pool) {
        sylves_free(path_pool);
        return NULL;
    }
    
    return path_pool;
}

void sylves_path_pool_destroy(SylvesPathPool* pool) {
    if (!pool) {
        return;
    }
    
    sylves_memory_pool_destroy(pool->pool);
    sylves_free(pool);
}

/* Generic pool implementation */

static size_t round_up_power_of_2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    n |= n >> 32;
#endif
    n++;
    return n;
}

SylvesGenericPool* sylves_generic_pool_create(
    size_t min_block_size,
    size_t max_block_size,
    bool thread_safe
) {
    if (min_block_size < sizeof(MemoryBlock) || max_block_size < min_block_size) {
        return NULL;
    }
    
    SylvesGenericPool* gpool = (SylvesGenericPool*)sylves_alloc(sizeof(SylvesGenericPool));
    if (!gpool) {
        return NULL;
    }
    
    /* Create buckets for power-of-2 sizes */
    min_block_size = round_up_power_of_2(min_block_size);
    max_block_size = round_up_power_of_2(max_block_size);
    
    size_t bucket_count = 0;
    for (size_t size = min_block_size; size <= max_block_size; size *= 2) {
        bucket_count++;
    }
    
    gpool->buckets = (GenericBucket*)sylves_alloc(sizeof(GenericBucket) * bucket_count);
    if (!gpool->buckets) {
        sylves_free(gpool);
        return NULL;
    }
    
    /* Initialize buckets */
    size_t i = 0;
    for (size_t size = min_block_size; size <= max_block_size; size *= 2) {
        SylvesPoolConfig config = {
            .block_size = size,
            .initial_capacity = 0,
            .max_capacity = 0,
            .thread_safe = thread_safe,
            .track_stats = false,
            .zero_on_alloc = false
        };
        
        gpool->buckets[i].size = size;
        gpool->buckets[i].pool = sylves_memory_pool_create(&config);
        if (!gpool->buckets[i].pool) {
            /* Clean up on failure */
            for (size_t j = 0; j < i; j++) {
                sylves_memory_pool_destroy(gpool->buckets[j].pool);
            }
            sylves_free(gpool->buckets);
            sylves_free(gpool);
            return NULL;
        }
        i++;
    }
    
    gpool->bucket_count = bucket_count;
    gpool->min_size = min_block_size;
    gpool->max_size = max_block_size;
    gpool->thread_safe = thread_safe;
    
    return gpool;
}

void sylves_generic_pool_destroy(SylvesGenericPool* pool) {
    if (!pool) {
        return;
    }
    
    for (size_t i = 0; i < pool->bucket_count; i++) {
        sylves_memory_pool_destroy(pool->buckets[i].pool);
    }
    
    sylves_free(pool->buckets);
    sylves_free(pool);
}

void* sylves_generic_pool_alloc(SylvesGenericPool* pool, size_t size) {
    if (!pool || size == 0) {
        return NULL;
    }
    
    /* Find appropriate bucket */
    size_t rounded_size = round_up_power_of_2(size);
    if (rounded_size < pool->min_size || rounded_size > pool->max_size) {
        /* Fall back to regular allocation */
        return sylves_alloc(size);
    }
    
    /* Find bucket */
    for (size_t i = 0; i < pool->bucket_count; i++) {
        if (pool->buckets[i].size >= rounded_size) {
            return sylves_pool_alloc(pool->buckets[i].pool);
        }
    }
    
    /* Should not reach here */
    return sylves_alloc(size);
}

void sylves_generic_pool_free(SylvesGenericPool* pool, void* ptr, size_t size) {
    if (!pool || !ptr) {
        return;
    }
    
    /* Find appropriate bucket */
    size_t rounded_size = round_up_power_of_2(size);
    if (rounded_size < pool->min_size || rounded_size > pool->max_size) {
        /* Was allocated with regular allocation */
        sylves_free(ptr);
        return;
    }
    
    /* Find bucket */
    for (size_t i = 0; i < pool->bucket_count; i++) {
        if (pool->buckets[i].size >= rounded_size) {
            sylves_pool_free(pool->buckets[i].pool, ptr);
            return;
        }
    }
    
    /* Should not reach here */
    sylves_free(ptr);
}

/* Thread-local pools */

SylvesCellPool* sylves_get_thread_cell_pool(void) {
    if (!tls_cell_pool) {
        tls_cell_pool = sylves_cell_pool_create(1024, false);
    }
    return tls_cell_pool;
}

SylvesPathPool* sylves_get_thread_path_pool(void) {
    if (!tls_path_pool) {
        tls_path_pool = sylves_path_pool_create(1024, false);
    }
    return tls_path_pool;
}
