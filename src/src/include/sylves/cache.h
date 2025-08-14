#ifndef SYLVES_CACHE_H
#define SYLVES_CACHE_H

#include "sylves/sylves.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cache Caching System
 * @brief Result caching for expensive computations
 * @{
 */

/**
 * Cache eviction policies
 */
typedef enum SylvesCachePolicy {
    SYLVES_CACHE_POLICY_LRU,      /**< Least Recently Used */
    SYLVES_CACHE_POLICY_LFU,      /**< Least Frequently Used */
    SYLVES_CACHE_POLICY_FIFO,     /**< First In First Out */
    SYLVES_CACHE_POLICY_RANDOM    /**< Random eviction */
} SylvesCachePolicy;

/**
 * Cache statistics
 */
typedef struct SylvesCacheStats {
    size_t total_entries;         /**< Total entries in cache */
    size_t memory_used;           /**< Total memory used by cache */
    size_t hit_count;             /**< Number of cache hits */
    size_t miss_count;            /**< Number of cache misses */
    size_t eviction_count;        /**< Number of evictions */
    double hit_rate;              /**< Hit rate percentage */
    double average_access_time_us; /**< Average access time in microseconds */
} SylvesCacheStats;

/**
 * Cache configuration
 */
typedef struct SylvesCacheConfig {
    size_t max_entries;           /**< Maximum number of entries (0 for unlimited) */
    size_t max_memory;            /**< Maximum memory usage in bytes (0 for unlimited) */
    SylvesCachePolicy policy;     /**< Eviction policy */
    bool thread_safe;             /**< Enable thread safety */
    bool track_stats;             /**< Enable statistics tracking */
} SylvesCacheConfig;

/**
 * Opaque cache structure
 */
typedef struct SylvesCache SylvesCache;

/**
 * Cell-based cache for grid computations
 */
typedef struct SylvesCellCache SylvesCellCache;

/**
 * Path cache for pathfinding results
 */
typedef struct SylvesPathCache SylvesPathCache;

/**
 * Mesh cache for expensive mesh generations
 */
typedef struct SylvesMeshCache SylvesMeshCache;

/**
 * Key hash function
 * @param key Key to hash
 * @param key_size Size of key in bytes
 * @return Hash value
 */
typedef size_t (*SylvesCacheHashFunc)(const void* key, size_t key_size);

/**
 * Key comparison function
 * @param key1 First key
 * @param key2 Second key
 * @param key_size Size of keys in bytes
 * @return 0 if equal, non-zero otherwise
 */
typedef int (*SylvesCacheCompareFunc)(const void* key1, const void* key2, size_t key_size);

/**
 * Value destructor function
 * @param value Value to destroy
 */
typedef void (*SylvesCacheDestroyFunc)(void* value);

/**
 * Value size calculation function
 * @param value Value to measure
 * @return Size in bytes
 */
typedef size_t (*SylvesCacheSizeFunc)(const void* value);

/* General cache functions */

/**
 * Create a cache with configuration
 * @param config Cache configuration
 * @param key_size Size of keys in bytes
 * @param hash_func Hash function for keys (NULL for default)
 * @param compare_func Comparison function for keys (NULL for default)
 * @param destroy_func Destructor for values (NULL if not needed)
 * @param size_func Size calculator for values (NULL for fixed size)
 * @return New cache or NULL on failure
 */
SYLVES_EXPORT SylvesCache* sylves_cache_create(
    const SylvesCacheConfig* config,
    size_t key_size,
    SylvesCacheHashFunc hash_func,
    SylvesCacheCompareFunc compare_func,
    SylvesCacheDestroyFunc destroy_func,
    SylvesCacheSizeFunc size_func
);

/**
 * Destroy cache and all entries
 * @param cache Cache to destroy
 */
SYLVES_EXPORT void sylves_cache_destroy(SylvesCache* cache);

/**
 * Get value from cache
 * @param cache Cache
 * @param key Key to lookup
 * @return Value or NULL if not found
 */
SYLVES_EXPORT void* sylves_cache_get(SylvesCache* cache, const void* key);

/**
 * Put value in cache
 * @param cache Cache
 * @param key Key to store
 * @param value Value to store
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_cache_put(SylvesCache* cache, const void* key, void* value);

/**
 * Remove entry from cache
 * @param cache Cache
 * @param key Key to remove
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_cache_remove(SylvesCache* cache, const void* key);

/**
 * Clear all entries from cache
 * @param cache Cache
 */
SYLVES_EXPORT void sylves_cache_clear(SylvesCache* cache);

/**
 * Get cache statistics
 * @param cache Cache
 * @param stats Output statistics
 */
SYLVES_EXPORT void sylves_cache_get_stats(const SylvesCache* cache, SylvesCacheStats* stats);

/**
 * Reset cache statistics
 * @param cache Cache
 */
SYLVES_EXPORT void sylves_cache_reset_stats(SylvesCache* cache);

/* Cell cache functions */

/**
 * Create a cell-based cache
 * @param grid Grid for cell operations
 * @param max_entries Maximum entries
 * @param thread_safe Enable thread safety
 * @return New cell cache or NULL on failure
 */
SYLVES_EXPORT SylvesCellCache* sylves_cell_cache_create(
    const SylvesGrid* grid,
    size_t max_entries,
    bool thread_safe
);

/**
 * Destroy cell cache
 * @param cache Cache to destroy
 */
SYLVES_EXPORT void sylves_cell_cache_destroy(SylvesCellCache* cache);

/**
 * Get cached mesh data for cell
 * @param cache Cell cache
 * @param cell Cell to lookup
 * @param out_mesh_data Output mesh data (do not free)
 * @param out_transform Output transform
 * @return true if found, false if miss
 */
SYLVES_EXPORT bool sylves_cell_cache_get_mesh(
    SylvesCellCache* cache,
    const SylvesCell* cell,
    SylvesMeshData** out_mesh_data,
    SylvesMatrix4x4* out_transform
);

/**
 * Cache mesh data for cell
 * @param cache Cell cache
 * @param cell Cell to cache
 * @param mesh_data Mesh data to cache (cache takes ownership)
 * @param transform Transform matrix
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_cell_cache_put_mesh(
    SylvesCellCache* cache,
    const SylvesCell* cell,
    SylvesMeshData* mesh_data,
    const SylvesMatrix4x4* transform
);

/**
 * Get cached polygon for cell
 * @param cache Cell cache
 * @param cell Cell to lookup
 * @param out_vertices Output vertices (do not free)
 * @param out_vertex_count Output vertex count
 * @param out_transform Output transform
 * @return true if found, false if miss
 */
SYLVES_EXPORT bool sylves_cell_cache_get_polygon(
    SylvesCellCache* cache,
    const SylvesCell* cell,
    SylvesVector3** out_vertices,
    int* out_vertex_count,
    SylvesMatrix4x4* out_transform
);

/**
 * Cache polygon for cell
 * @param cache Cell cache
 * @param cell Cell to cache
 * @param vertices Polygon vertices
 * @param vertex_count Number of vertices
 * @param transform Transform matrix
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_cell_cache_put_polygon(
    SylvesCellCache* cache,
    const SylvesCell* cell,
    const SylvesVector3* vertices,
    int vertex_count,
    const SylvesMatrix4x4* transform
);

/* Path cache functions */

/**
 * Create a path cache
 * @param max_entries Maximum entries
 * @param thread_safe Enable thread safety
 * @return New path cache or NULL on failure
 */
SYLVES_EXPORT SylvesPathCache* sylves_path_cache_create(
    size_t max_entries,
    bool thread_safe
);

/**
 * Destroy path cache
 * @param cache Cache to destroy
 */
SYLVES_EXPORT void sylves_path_cache_destroy(SylvesPathCache* cache);

/**
 * Get cached path
 * @param cache Path cache
 * @param start Start cell
 * @param goal Goal cell
 * @return Cached path or NULL if not found
 */
SYLVES_EXPORT SylvesCellPath* sylves_path_cache_get(
    SylvesPathCache* cache,
    const SylvesCell* start,
    const SylvesCell* goal
);

/**
 * Cache a path
 * @param cache Path cache
 * @param start Start cell
 * @param goal Goal cell
 * @param path Path to cache (cache takes ownership)
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_path_cache_put(
    SylvesPathCache* cache,
    const SylvesCell* start,
    const SylvesCell* goal,
    SylvesCellPath* path
);

/**
 * Invalidate paths containing a specific cell
 * @param cache Path cache
 * @param cell Cell that changed
 */
SYLVES_EXPORT void sylves_path_cache_invalidate_cell(
    SylvesPathCache* cache,
    const SylvesCell* cell
);

/* Mesh cache functions */

/**
 * Create a mesh cache
 * @param max_memory Maximum memory usage
 * @param thread_safe Enable thread safety
 * @return New mesh cache or NULL on failure
 */
SYLVES_EXPORT SylvesMeshCache* sylves_mesh_cache_create(
    size_t max_memory,
    bool thread_safe
);

/**
 * Destroy mesh cache
 * @param cache Cache to destroy
 */
SYLVES_EXPORT void sylves_mesh_cache_destroy(SylvesMeshCache* cache);

/**
 * Get cached mesh by ID
 * @param cache Mesh cache
 * @param mesh_id Mesh identifier
 * @return Cached mesh or NULL if not found
 */
SYLVES_EXPORT SylvesMeshData* sylves_mesh_cache_get(
    SylvesMeshCache* cache,
    uint64_t mesh_id
);

/**
 * Cache a mesh
 * @param cache Mesh cache
 * @param mesh_id Mesh identifier
 * @param mesh Mesh to cache (cache takes ownership)
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_mesh_cache_put(
    SylvesMeshCache* cache,
    uint64_t mesh_id,
    SylvesMeshData* mesh
);

/* Cache policy implementations */

/**
 * Create default cache policy for grids
 * @param grid Grid to cache for
 * @return Recommended cache configuration
 */
SYLVES_EXPORT SylvesCacheConfig sylves_cache_policy_always(const SylvesGrid* grid);

/**
 * Create concurrent cache policy
 * @param grid Grid to cache for
 * @return Thread-safe cache configuration
 */
SYLVES_EXPORT SylvesCacheConfig sylves_cache_policy_concurrent_always(const SylvesGrid* grid);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_CACHE_H */
