#ifndef SYLVES_MEMORY_POOL_H
#define SYLVES_MEMORY_POOL_H

#include "sylves/sylves.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup memory_pool Memory Pool System
 * @brief High-performance memory pool allocators for frequent allocations
 * @{
 */

/**
 * Memory pool statistics
 */
typedef struct SylvesPoolStats {
    size_t total_allocations;      /**< Total number of allocations */
    size_t active_allocations;     /**< Current active allocations */
    size_t total_memory_used;      /**< Total memory currently allocated */
    size_t peak_memory_used;       /**< Peak memory usage */
    size_t reuse_count;           /**< Number of times blocks were reused */
} SylvesPoolStats;

/**
 * Memory pool configuration
 */
typedef struct SylvesPoolConfig {
    size_t block_size;            /**< Size of each allocation block */
    size_t initial_capacity;      /**< Initial number of blocks to pre-allocate */
    size_t max_capacity;          /**< Maximum number of blocks (0 for unlimited) */
    bool thread_safe;             /**< Enable thread-safe operations */
    bool track_stats;             /**< Enable statistics tracking */
    bool zero_on_alloc;           /**< Zero memory on allocation */
} SylvesPoolConfig;

/**
 * Opaque memory pool structure
 */
typedef struct SylvesMemoryPool SylvesMemoryPool;

/**
 * Cell pool type for frequent cell allocations
 */
typedef struct SylvesCellPool SylvesCellPool;

/**
 * Path pool type for pathfinding operations
 */
typedef struct SylvesPathPool SylvesPathPool;

/**
 * Generic memory pool type
 */
typedef struct SylvesGenericPool SylvesGenericPool;

/* Pool creation and destruction */

/**
 * Create a memory pool with specified configuration
 * @param config Pool configuration
 * @return New memory pool or NULL on failure
 */
SYLVES_EXPORT SylvesMemoryPool* sylves_memory_pool_create(const SylvesPoolConfig* config);

/**
 * Destroy a memory pool and free all resources
 * @param pool Pool to destroy
 */
SYLVES_EXPORT void sylves_memory_pool_destroy(SylvesMemoryPool* pool);

/**
 * Create a specialized cell pool
 * @param initial_capacity Initial number of cells to pre-allocate
 * @param thread_safe Enable thread safety
 * @return New cell pool or NULL on failure
 */
SYLVES_EXPORT SylvesCellPool* sylves_cell_pool_create(size_t initial_capacity, bool thread_safe);

/**
 * Destroy a cell pool
 * @param pool Pool to destroy
 */
SYLVES_EXPORT void sylves_cell_pool_destroy(SylvesCellPool* pool);

/**
 * Create a specialized path pool
 * @param initial_capacity Initial number of path nodes to pre-allocate
 * @param thread_safe Enable thread safety
 * @return New path pool or NULL on failure
 */
SYLVES_EXPORT SylvesPathPool* sylves_path_pool_create(size_t initial_capacity, bool thread_safe);

/**
 * Destroy a path pool
 * @param pool Pool to destroy
 */
SYLVES_EXPORT void sylves_path_pool_destroy(SylvesPathPool* pool);

/* Memory allocation and deallocation */

/**
 * Allocate memory from pool
 * @param pool Memory pool
 * @return Pointer to allocated memory or NULL if pool is exhausted
 */
SYLVES_EXPORT void* sylves_pool_alloc(SylvesMemoryPool* pool);

/**
 * Return memory to pool
 * @param pool Memory pool
 * @param ptr Memory to return (must have been allocated from this pool)
 */
SYLVES_EXPORT void sylves_pool_free(SylvesMemoryPool* pool, void* ptr);

/**
 * Allocate a cell from cell pool
 * @param pool Cell pool
 * @return Pointer to cell or NULL if pool is exhausted
 */
SYLVES_EXPORT SylvesCell* sylves_cell_pool_alloc(SylvesCellPool* pool);

/**
 * Return cell to pool
 * @param pool Cell pool
 * @param cell Cell to return
 */
SYLVES_EXPORT void sylves_cell_pool_free(SylvesCellPool* pool, SylvesCell* cell);

/**
 * Allocate array of cells from pool
 * @param pool Cell pool
 * @param count Number of cells to allocate
 * @return Pointer to cell array or NULL if insufficient space
 */
SYLVES_EXPORT SylvesCell* sylves_cell_pool_alloc_array(SylvesCellPool* pool, size_t count);

/**
 * Free array of cells back to pool
 * @param pool Cell pool
 * @param cells Array of cells
 * @param count Number of cells
 */
SYLVES_EXPORT void sylves_cell_pool_free_array(SylvesCellPool* pool, SylvesCell* cells, size_t count);

/* Pool management */

/**
 * Reset pool, returning all allocations
 * @param pool Memory pool
 */
SYLVES_EXPORT void sylves_pool_reset(SylvesMemoryPool* pool);

/**
 * Get pool statistics
 * @param pool Memory pool
 * @param stats Output statistics structure
 */
SYLVES_EXPORT void sylves_pool_get_stats(const SylvesMemoryPool* pool, SylvesPoolStats* stats);

/**
 * Trim pool to reduce memory usage
 * @param pool Memory pool
 * @param target_capacity Target capacity (0 to trim to current usage)
 */
SYLVES_EXPORT void sylves_pool_trim(SylvesMemoryPool* pool, size_t target_capacity);

/* Generic pool for variable-sized allocations */

/**
 * Create a generic pool for variable-sized allocations
 * @param min_block_size Minimum block size
 * @param max_block_size Maximum block size
 * @param thread_safe Enable thread safety
 * @return New generic pool or NULL on failure
 */
SYLVES_EXPORT SylvesGenericPool* sylves_generic_pool_create(
    size_t min_block_size,
    size_t max_block_size,
    bool thread_safe
);

/**
 * Destroy generic pool
 * @param pool Pool to destroy
 */
SYLVES_EXPORT void sylves_generic_pool_destroy(SylvesGenericPool* pool);

/**
 * Allocate from generic pool
 * @param pool Generic pool
 * @param size Size to allocate
 * @return Allocated memory or NULL
 */
SYLVES_EXPORT void* sylves_generic_pool_alloc(SylvesGenericPool* pool, size_t size);

/**
 * Free to generic pool
 * @param pool Generic pool
 * @param ptr Memory to free
 * @param size Size of allocation
 */
SYLVES_EXPORT void sylves_generic_pool_free(SylvesGenericPool* pool, void* ptr, size_t size);

/* Thread-local pools */

/**
 * Get thread-local cell pool
 * @return Thread-local cell pool (created on first access)
 */
SYLVES_EXPORT SylvesCellPool* sylves_get_thread_cell_pool(void);

/**
 * Get thread-local path pool
 * @return Thread-local path pool (created on first access)
 */
SYLVES_EXPORT SylvesPathPool* sylves_get_thread_path_pool(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_MEMORY_POOL_H */
