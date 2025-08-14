#include "sylves/cache.h"
#include "sylves/memory.h"
#include "sylves/pathfinding.h"
#include "sylves/mesh_data.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define GET_TIME_US() (GetTickCount64() * 1000)
#else
#include <pthread.h>
#include <sys/time.h>
static uint64_t GET_TIME_US() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

/**
 * Cache entry for LRU tracking
 */
typedef struct CacheEntry {
    void* key;                    /**< Entry key */
    void* value;                  /**< Cached value */
    size_t value_size;            /**< Size of value */
    uint64_t last_access;         /**< Last access timestamp */
    uint64_t access_count;        /**< Access count for LFU */
    struct CacheEntry* prev;      /**< Previous in LRU list */
    struct CacheEntry* next;      /**< Next in LRU list */
    struct CacheEntry* hash_next; /**< Next in hash bucket */
} CacheEntry;

/**
 * General cache implementation
 */
struct SylvesCache {
    CacheEntry** buckets;         /**< Hash table buckets */
    size_t bucket_count;          /**< Number of buckets */
    CacheEntry* lru_head;         /**< Most recently used */
    CacheEntry* lru_tail;         /**< Least recently used */
    size_t entry_count;           /**< Current entries */
    size_t memory_used;           /**< Current memory usage */
    SylvesCacheConfig config;     /**< Cache configuration */
    size_t key_size;              /**< Size of keys */
    SylvesCacheHashFunc hash_func;
    SylvesCacheCompareFunc compare_func;
    SylvesCacheDestroyFunc destroy_func;
    SylvesCacheSizeFunc size_func;
    SylvesCacheStats stats;       /**< Cache statistics */
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

/**
 * Cell cache implementation
 */
struct SylvesCellCache {
    SylvesCache* mesh_cache;      /**< Cache for mesh data */
    SylvesCache* polygon_cache;   /**< Cache for polygons */
    const SylvesGrid* grid;       /**< Associated grid */
};

/**
 * Cached mesh data
 */
typedef struct CachedMeshData {
    SylvesMeshData* mesh_data;
    SylvesMatrix4x4 transform;
} CachedMeshData;

/**
 * Cached polygon data
 */
typedef struct CachedPolygon {
    SylvesVector3* vertices;
    int vertex_count;
    SylvesMatrix4x4 transform;
} CachedPolygon;

/**
 * Path cache key
 */
typedef struct PathCacheKey {
    SylvesCell start;
    SylvesCell goal;
} PathCacheKey;

/**
 * Path cache implementation
 */
struct SylvesPathCache {
    SylvesCache* cache;           /**< Underlying cache */
};

/**
 * Mesh cache implementation
 */
struct SylvesMeshCache {
    SylvesCache* cache;           /**< Underlying cache */
};

/* Helper functions */

static void init_lock(SylvesCache* cache) {
    if (cache->config.thread_safe) {
#ifdef _WIN32
        InitializeCriticalSection(&cache->lock);
#else
        pthread_mutex_init(&cache->lock, NULL);
#endif
    }
}

static void destroy_lock(SylvesCache* cache) {
    if (cache->config.thread_safe) {
#ifdef _WIN32
        DeleteCriticalSection(&cache->lock);
#else
        pthread_mutex_destroy(&cache->lock);
#endif
    }
}

static void lock_cache(SylvesCache* cache) {
    if (cache->config.thread_safe) {
#ifdef _WIN32
        EnterCriticalSection(&cache->lock);
#else
        pthread_mutex_lock(&cache->lock);
#endif
    }
}

static void unlock_cache(SylvesCache* cache) {
    if (cache->config.thread_safe) {
#ifdef _WIN32
        LeaveCriticalSection(&cache->lock);
#else
        pthread_mutex_unlock(&cache->lock);
#endif
    }
}

/* Default hash function */
static size_t default_hash(const void* key, size_t key_size) {
    const uint8_t* bytes = (const uint8_t*)key;
    size_t hash = 5381;
    for (size_t i = 0; i < key_size; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    return hash;
}

/* Default compare function */
static int default_compare(const void* key1, const void* key2, size_t key_size) {
    return memcmp(key1, key2, key_size);
}

/* LRU list operations */
static void lru_remove(SylvesCache* cache, CacheEntry* entry) {
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        cache->lru_head = entry->next;
    }
    
    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        cache->lru_tail = entry->prev;
    }
}

static void lru_add_front(SylvesCache* cache, CacheEntry* entry) {
    entry->prev = NULL;
    entry->next = cache->lru_head;
    
    if (cache->lru_head) {
        cache->lru_head->prev = entry;
    }
    cache->lru_head = entry;
    
    if (!cache->lru_tail) {
        cache->lru_tail = entry;
    }
}

static void lru_move_front(SylvesCache* cache, CacheEntry* entry) {
    if (entry != cache->lru_head) {
        lru_remove(cache, entry);
        lru_add_front(cache, entry);
    }
}

/* Entry operations */
static CacheEntry* find_entry(SylvesCache* cache, const void* key) {
    size_t hash = cache->hash_func(key, cache->key_size);
    size_t bucket_idx = hash % cache->bucket_count;
    
    CacheEntry* entry = cache->buckets[bucket_idx];
    while (entry) {
        if (cache->compare_func(entry->key, key, cache->key_size) == 0) {
            return entry;
        }
        entry = entry->hash_next;
    }
    
    return NULL;
}

static void remove_entry(SylvesCache* cache, CacheEntry* entry) {
    /* Remove from hash table */
    size_t hash = cache->hash_func(entry->key, cache->key_size);
    size_t bucket_idx = hash % cache->bucket_count;
    
    CacheEntry** prev_next = &cache->buckets[bucket_idx];
    CacheEntry* curr = cache->buckets[bucket_idx];
    
    while (curr) {
        if (curr == entry) {
            *prev_next = curr->hash_next;
            break;
        }
        prev_next = &curr->hash_next;
        curr = curr->hash_next;
    }
    
    /* Remove from LRU list */
    lru_remove(cache, entry);
    
    /* Update stats */
    cache->entry_count--;
    cache->memory_used -= entry->value_size;
    
    /* Destroy value */
    if (cache->destroy_func) {
        cache->destroy_func(entry->value);
    }
    
    /* Free entry */
    sylves_free(entry->key);
    sylves_free(entry);
}

static CacheEntry* evict_entry(SylvesCache* cache) {
    CacheEntry* victim = NULL;
    
    switch (cache->config.policy) {
        case SYLVES_CACHE_POLICY_LRU:
            victim = cache->lru_tail;
            break;
            
        case SYLVES_CACHE_POLICY_LFU: {
            /* Find least frequently used */
            uint64_t min_count = UINT64_MAX;
            for (CacheEntry* entry = cache->lru_head; entry; entry = entry->next) {
                if (entry->access_count < min_count) {
                    min_count = entry->access_count;
                    victim = entry;
                }
            }
            break;
        }
        
        case SYLVES_CACHE_POLICY_FIFO:
            /* Same as LRU but without moving on access */
            victim = cache->lru_tail;
            break;
            
        case SYLVES_CACHE_POLICY_RANDOM: {
            /* Pick random entry */
            size_t idx = rand() % cache->entry_count;
            victim = cache->lru_head;
            for (size_t i = 0; i < idx && victim; i++) {
                victim = victim->next;
            }
            break;
        }
    }
    
    if (victim) {
        if (cache->config.track_stats) {
            cache->stats.eviction_count++;
        }
        remove_entry(cache, victim);
    }
    
    return victim;
}

/* Public API implementation */

SylvesCache* sylves_cache_create(
    const SylvesCacheConfig* config,
    size_t key_size,
    SylvesCacheHashFunc hash_func,
    SylvesCacheCompareFunc compare_func,
    SylvesCacheDestroyFunc destroy_func,
    SylvesCacheSizeFunc size_func
) {
    if (!config || key_size == 0) {
        return NULL;
    }
    
    SylvesCache* cache = (SylvesCache*)sylves_alloc(sizeof(SylvesCache));
    if (!cache) {
        return NULL;
    }
    
    memset(cache, 0, sizeof(SylvesCache));
    cache->config = *config;
    cache->key_size = key_size;
    cache->hash_func = hash_func ? hash_func : default_hash;
    cache->compare_func = compare_func ? compare_func : default_compare;
    cache->destroy_func = destroy_func;
    cache->size_func = size_func;
    
    /* Initialize hash table */
    cache->bucket_count = 1024; /* Default size */
    cache->buckets = (CacheEntry**)sylves_alloc(sizeof(CacheEntry*) * cache->bucket_count);
    if (!cache->buckets) {
        sylves_free(cache);
        return NULL;
    }
    memset(cache->buckets, 0, sizeof(CacheEntry*) * cache->bucket_count);
    
    init_lock(cache);
    
    return cache;
}

void sylves_cache_destroy(SylvesCache* cache) {
    if (!cache) {
        return;
    }
    
    /* Clear all entries */
    sylves_cache_clear(cache);
    
    destroy_lock(cache);
    sylves_free(cache->buckets);
    sylves_free(cache);
}

void* sylves_cache_get(SylvesCache* cache, const void* key) {
    if (!cache || !key) {
        return NULL;
    }
    
    uint64_t start_time = 0;
    if (cache->config.track_stats) {
        start_time = GET_TIME_US();
    }
    
    lock_cache(cache);
    
    CacheEntry* entry = find_entry(cache, key);
    void* value = NULL;
    
    if (entry) {
        /* Cache hit */
        value = entry->value;
        entry->last_access = GET_TIME_US();
        entry->access_count++;
        
        if (cache->config.policy == SYLVES_CACHE_POLICY_LRU) {
            lru_move_front(cache, entry);
        }
        
        if (cache->config.track_stats) {
            cache->stats.hit_count++;
        }
    } else {
        /* Cache miss */
        if (cache->config.track_stats) {
            cache->stats.miss_count++;
        }
    }
    
    unlock_cache(cache);
    
    if (cache->config.track_stats) {
        uint64_t elapsed = GET_TIME_US() - start_time;
        cache->stats.average_access_time_us = 
            (cache->stats.average_access_time_us * (cache->stats.hit_count + cache->stats.miss_count - 1) + elapsed) /
            (cache->stats.hit_count + cache->stats.miss_count);
    }
    
    return value;
}

SylvesError sylves_cache_put(SylvesCache* cache, const void* key, void* value) {
    if (!cache || !key || !value) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    lock_cache(cache);
    
    /* Check if key exists */
    CacheEntry* entry = find_entry(cache, key);
    if (entry) {
        /* Update existing entry */
        if (cache->destroy_func) {
            cache->destroy_func(entry->value);
        }
        
        cache->memory_used -= entry->value_size;
        entry->value = value;
        entry->value_size = cache->size_func ? cache->size_func(value) : 0;
        cache->memory_used += entry->value_size;
        
        entry->last_access = GET_TIME_US();
        entry->access_count++;
        
        if (cache->config.policy == SYLVES_CACHE_POLICY_LRU) {
            lru_move_front(cache, entry);
        }
    } else {
        /* Create new entry */
        size_t value_size = cache->size_func ? cache->size_func(value) : 0;
        
        /* Check limits and evict if necessary */
        while ((cache->config.max_entries > 0 && cache->entry_count >= cache->config.max_entries) ||
               (cache->config.max_memory > 0 && cache->memory_used + value_size > cache->config.max_memory)) {
            if (!evict_entry(cache)) {
                unlock_cache(cache);
                return SYLVES_ERROR_OUT_OF_MEMORY;
            }
        }
        
        /* Allocate new entry */
        entry = (CacheEntry*)sylves_alloc(sizeof(CacheEntry));
        if (!entry) {
            unlock_cache(cache);
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
        
        entry->key = sylves_alloc(cache->key_size);
        if (!entry->key) {
            sylves_free(entry);
            unlock_cache(cache);
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(entry->key, key, cache->key_size);
        entry->value = value;
        entry->value_size = value_size;
        entry->last_access = GET_TIME_US();
        entry->access_count = 1;
        
        /* Add to hash table */
        size_t hash = cache->hash_func(key, cache->key_size);
        size_t bucket_idx = hash % cache->bucket_count;
        entry->hash_next = cache->buckets[bucket_idx];
        cache->buckets[bucket_idx] = entry;
        
        /* Add to LRU list */
        lru_add_front(cache, entry);
        
        /* Update stats */
        cache->entry_count++;
        cache->memory_used += value_size;
        cache->stats.total_entries = cache->entry_count;
        cache->stats.memory_used = cache->memory_used;
    }
    
    unlock_cache(cache);
    
    return SYLVES_SUCCESS;
}

SylvesError sylves_cache_remove(SylvesCache* cache, const void* key) {
    if (!cache || !key) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    lock_cache(cache);
    
    CacheEntry* entry = find_entry(cache, key);
    if (!entry) {
        unlock_cache(cache);
return SYLVES_ERROR_CELL_NOT_FOUND;
    }
    
    remove_entry(cache, entry);
    
    unlock_cache(cache);
    
    return SYLVES_SUCCESS;
}

void sylves_cache_clear(SylvesCache* cache) {
    if (!cache) {
        return;
    }
    
    lock_cache(cache);
    
    /* Remove all entries */
    while (cache->lru_head) {
        remove_entry(cache, cache->lru_head);
    }
    
    unlock_cache(cache);
}

void sylves_cache_get_stats(const SylvesCache* cache, SylvesCacheStats* stats) {
    if (!cache || !stats) {
        return;
    }
    
    *stats = cache->stats;
    stats->total_entries = cache->entry_count;
    stats->memory_used = cache->memory_used;
    
    if (cache->stats.hit_count + cache->stats.miss_count > 0) {
        stats->hit_rate = (double)cache->stats.hit_count / 
                         (cache->stats.hit_count + cache->stats.miss_count) * 100.0;
    }
}

void sylves_cache_reset_stats(SylvesCache* cache) {
    if (!cache) {
        return;
    }
    
    lock_cache(cache);
    memset(&cache->stats, 0, sizeof(SylvesCacheStats));
    unlock_cache(cache);
}

/* Cell cache implementation */

static size_t cell_hash(const void* key, size_t key_size) {
    const SylvesCell* cell = (const SylvesCell*)key;
    return (size_t)(cell->x * 73856093) ^ (size_t)(cell->y * 19349663) ^ (size_t)(cell->z * 83492791);
}

static int cell_compare(const void* key1, const void* key2, size_t key_size) {
    (void)key_size;
    const SylvesCell* c1 = (const SylvesCell*)key1;
    const SylvesCell* c2 = (const SylvesCell*)key2;
    return sylves_cell_equals(*c1, *c2) ? 0 : 1;
}

static void cached_mesh_destroy(void* value) {
    CachedMeshData* data = (CachedMeshData*)value;
    if (data) {
        sylves_mesh_data_destroy(data->mesh_data);
        sylves_free(data);
    }
}

static size_t cached_mesh_size(const void* value) {
    const CachedMeshData* data = (const CachedMeshData*)value;
    if (!data || !data->mesh_data) {
        return sizeof(CachedMeshData);
    }
    
    /* Estimate mesh size */
    return sizeof(CachedMeshData) + 
           data->mesh_data->vertex_count * sizeof(SylvesVector3) +
           data->mesh_data->face_count * sizeof(SylvesMeshFace);
}

static void cached_polygon_destroy(void* value) {
    CachedPolygon* data = (CachedPolygon*)value;
    if (data) {
        sylves_free(data->vertices);
        sylves_free(data);
    }
}

static size_t cached_polygon_size(const void* value) {
    const CachedPolygon* data = (const CachedPolygon*)value;
    return sizeof(CachedPolygon) + 
           (data ? data->vertex_count * sizeof(SylvesVector3) : 0);
}

SylvesCellCache* sylves_cell_cache_create(const SylvesGrid* grid, size_t max_entries, bool thread_safe) {
    if (!grid) {
        return NULL;
    }
    
    SylvesCellCache* cache = (SylvesCellCache*)sylves_alloc(sizeof(SylvesCellCache));
    if (!cache) {
        return NULL;
    }
    
    SylvesCacheConfig config = {
        .max_entries = max_entries / 2, /* Split between mesh and polygon */
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = thread_safe,
        .track_stats = true
    };
    
    cache->mesh_cache = sylves_cache_create(&config, sizeof(SylvesCell), 
                                           cell_hash, cell_compare,
                                           cached_mesh_destroy, cached_mesh_size);
    
    cache->polygon_cache = sylves_cache_create(&config, sizeof(SylvesCell),
                                              cell_hash, cell_compare,
                                              cached_polygon_destroy, cached_polygon_size);
    
    if (!cache->mesh_cache || !cache->polygon_cache) {
        sylves_cache_destroy(cache->mesh_cache);
        sylves_cache_destroy(cache->polygon_cache);
        sylves_free(cache);
        return NULL;
    }
    
    cache->grid = grid;
    
    return cache;
}

void sylves_cell_cache_destroy(SylvesCellCache* cache) {
    if (!cache) {
        return;
    }
    
    sylves_cache_destroy(cache->mesh_cache);
    sylves_cache_destroy(cache->polygon_cache);
    sylves_free(cache);
}

bool sylves_cell_cache_get_mesh(SylvesCellCache* cache, const SylvesCell* cell,
                               SylvesMeshData** out_mesh_data, SylvesMatrix4x4* out_transform) {
    if (!cache || !cell || !out_mesh_data || !out_transform) {
        return false;
    }
    
    CachedMeshData* data = (CachedMeshData*)sylves_cache_get(cache->mesh_cache, cell);
    if (!data) {
        return false;
    }
    
    *out_mesh_data = data->mesh_data;
    *out_transform = data->transform;
    return true;
}

SylvesError sylves_cell_cache_put_mesh(SylvesCellCache* cache, const SylvesCell* cell,
                                      SylvesMeshData* mesh_data, const SylvesMatrix4x4* transform) {
    if (!cache || !cell || !mesh_data || !transform) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    CachedMeshData* data = (CachedMeshData*)sylves_alloc(sizeof(CachedMeshData));
    if (!data) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    data->mesh_data = mesh_data;
    data->transform = *transform;
    
    return sylves_cache_put(cache->mesh_cache, cell, data);
}

bool sylves_cell_cache_get_polygon(SylvesCellCache* cache, const SylvesCell* cell,
                                  SylvesVector3** out_vertices, int* out_vertex_count,
                                  SylvesMatrix4x4* out_transform) {
    if (!cache || !cell || !out_vertices || !out_vertex_count || !out_transform) {
        return false;
    }
    
    CachedPolygon* data = (CachedPolygon*)sylves_cache_get(cache->polygon_cache, cell);
    if (!data) {
        return false;
    }
    
    *out_vertices = data->vertices;
    *out_vertex_count = data->vertex_count;
    *out_transform = data->transform;
    return true;
}

SylvesError sylves_cell_cache_put_polygon(SylvesCellCache* cache, const SylvesCell* cell,
                                         const SylvesVector3* vertices, int vertex_count,
                                         const SylvesMatrix4x4* transform) {
    if (!cache || !cell || !vertices || vertex_count <= 0 || !transform) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    CachedPolygon* data = (CachedPolygon*)sylves_alloc(sizeof(CachedPolygon));
    if (!data) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    data->vertices = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * vertex_count);
    if (!data->vertices) {
        sylves_free(data);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(data->vertices, vertices, sizeof(SylvesVector3) * vertex_count);
    data->vertex_count = vertex_count;
    data->transform = *transform;
    
    return sylves_cache_put(cache->polygon_cache, cell, data);
}

/* Path cache implementation */

static size_t path_key_hash(const void* key, size_t key_size) {
    const PathCacheKey* pk = (const PathCacheKey*)key;
    size_t h1 = cell_hash(&pk->start, sizeof(SylvesCell));
    size_t h2 = cell_hash(&pk->goal, sizeof(SylvesCell));
    return h1 ^ (h2 << 1);
}

static int path_key_compare(const void* key1, const void* key2, size_t key_size) {
    const PathCacheKey* pk1 = (const PathCacheKey*)key1;
    const PathCacheKey* pk2 = (const PathCacheKey*)key2;
return (sylves_cell_equals(pk1->start, pk2->start) &&
            sylves_cell_equals(pk1->goal, pk2->goal)) ? 0 : 1;
}

static void path_destroy(void* value) {
    sylves_cell_path_destroy((SylvesCellPath*)value);
}

static size_t path_size(const void* value) {
    const SylvesCellPath* path = (const SylvesCellPath*)value;
    return sizeof(SylvesCellPath) + 
           (path ? path->step_count * sizeof(SylvesStep) : 0);
}

SylvesPathCache* sylves_path_cache_create(size_t max_entries, bool thread_safe) {
    SylvesPathCache* cache = (SylvesPathCache*)sylves_alloc(sizeof(SylvesPathCache));
    if (!cache) {
        return NULL;
    }
    
    SylvesCacheConfig config = {
        .max_entries = max_entries,
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = thread_safe,
        .track_stats = true
    };
    
    cache->cache = sylves_cache_create(&config, sizeof(PathCacheKey),
                                      path_key_hash, path_key_compare,
                                      path_destroy, path_size);
    
    if (!cache->cache) {
        sylves_free(cache);
        return NULL;
    }
    
    return cache;
}

void sylves_path_cache_destroy(SylvesPathCache* cache) {
    if (!cache) {
        return;
    }
    
    sylves_cache_destroy(cache->cache);
    sylves_free(cache);
}

SylvesCellPath* sylves_path_cache_get(SylvesPathCache* cache, const SylvesCell* start, const SylvesCell* goal) {
    if (!cache || !start || !goal) {
        return NULL;
    }
    
    PathCacheKey key = { *start, *goal };
    return (SylvesCellPath*)sylves_cache_get(cache->cache, &key);
}

SylvesError sylves_path_cache_put(SylvesPathCache* cache, const SylvesCell* start,
                                  const SylvesCell* goal, SylvesCellPath* path) {
    if (!cache || !start || !goal || !path) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    PathCacheKey key = { *start, *goal };
    return sylves_cache_put(cache->cache, &key, path);
}

void sylves_path_cache_invalidate_cell(SylvesPathCache* cache, const SylvesCell* cell) {
    if (!cache || !cell) {
        return;
    }
    
    /* This would require iterating all entries to find paths containing the cell
     * For now, we just clear the entire cache as a simple solution */
    sylves_cache_clear(cache->cache);
}

/* Mesh cache implementation */

static size_t uint64_hash(const void* key, size_t key_size) {
    uint64_t id = *(const uint64_t*)key;
    id = (~id) + (id << 21);
    id = id ^ (id >> 24);
    id = (id + (id << 3)) + (id << 8);
    id = id ^ (id >> 14);
    id = (id + (id << 2)) + (id << 4);
    id = id ^ (id >> 28);
    id = id + (id << 31);
    return (size_t)id;
}

static int uint64_compare(const void* key1, const void* key2, size_t key_size) {
    uint64_t id1 = *(const uint64_t*)key1;
    uint64_t id2 = *(const uint64_t*)key2;
    return (id1 == id2) ? 0 : 1;
}

static void mesh_destroy(void* value) {
    sylves_mesh_data_destroy((SylvesMeshData*)value);
}

static size_t mesh_size(const void* value) {
    const SylvesMeshData* mesh = (const SylvesMeshData*)value;
    if (!mesh) {
        return 0;
    }
    
    return sizeof(SylvesMeshData) +
           mesh->vertex_count * sizeof(SylvesVector3) +
           mesh->face_count * sizeof(SylvesMeshFace);
}

SylvesMeshCache* sylves_mesh_cache_create(size_t max_memory, bool thread_safe) {
    SylvesMeshCache* cache = (SylvesMeshCache*)sylves_alloc(sizeof(SylvesMeshCache));
    if (!cache) {
        return NULL;
    }
    
    SylvesCacheConfig config = {
        .max_entries = 0,
        .max_memory = max_memory,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = thread_safe,
        .track_stats = true
    };
    
    cache->cache = sylves_cache_create(&config, sizeof(uint64_t),
                                      uint64_hash, uint64_compare,
                                      mesh_destroy, mesh_size);
    
    if (!cache->cache) {
        sylves_free(cache);
        return NULL;
    }
    
    return cache;
}

void sylves_mesh_cache_destroy(SylvesMeshCache* cache) {
    if (!cache) {
        return;
    }
    
    sylves_cache_destroy(cache->cache);
    sylves_free(cache);
}

SylvesMeshData* sylves_mesh_cache_get(SylvesMeshCache* cache, uint64_t mesh_id) {
    if (!cache) {
        return NULL;
    }
    
    return (SylvesMeshData*)sylves_cache_get(cache->cache, &mesh_id);
}

SylvesError sylves_mesh_cache_put(SylvesMeshCache* cache, uint64_t mesh_id, SylvesMeshData* mesh) {
    if (!cache || !mesh) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    return sylves_cache_put(cache->cache, &mesh_id, mesh);
}

/* Cache policy implementations */

SylvesCacheConfig sylves_cache_policy_always(const SylvesGrid* grid) {
    SylvesCacheConfig config = {
        .max_entries = 10000,     /* Default large cache */
        .max_memory = 100 * 1024 * 1024, /* 100MB */
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = false,
        .track_stats = true
    };
    
    return config;
}

SylvesCacheConfig sylves_cache_policy_concurrent_always(const SylvesGrid* grid) {
    SylvesCacheConfig config = sylves_cache_policy_always(grid);
    config.thread_safe = true;
    return config;
}
