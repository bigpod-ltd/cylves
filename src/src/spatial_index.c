#include "sylves/spatial_index.h"
#include "sylves/memory.h"
#include "sylves/grid.h"
#include "sylves/hash.h"
#include <string.h>
#include <math.h>
#include <float.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

/**
 * Hash entry for storing cells
 */
typedef struct HashEntry {
    SylvesCell cell;
    SylvesVector3 center;
    void* data;
    struct HashEntry* next;
} HashEntry;

/**
 * Spatial hash bucket
 */
typedef struct HashBucket {
    HashEntry* entries;
    size_t count;
} HashBucket;

/**
 * Grid-based spatial hash implementation
 */
typedef struct GridHashIndex {
    HashBucket* buckets;
    size_t bucket_count;
    double cell_size;
    double inv_cell_size;
    size_t item_count;
    SylvesHash* cell_to_bucket; /* Maps cell to bucket index for removal */
} GridHashIndex;

/**
 * General spatial index structure
 */
struct SylvesSpatialIndex {
    SylvesSpatialIndexType type;
    int dimension;
    union {
        GridHashIndex* grid_hash;
        /* Other index types would go here */
    } data;
    SylvesSpatialIndexStats stats;
    bool thread_safe;
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

/**
 * Grid spatial hash structure
 */
struct SylvesGridSpatialHash {
    SylvesSpatialIndex* index;
    const SylvesGrid* grid;
    double cell_size;
};

/* Helper functions */

static void init_lock(SylvesSpatialIndex* index) {
    if (index->thread_safe) {
#ifdef _WIN32
        InitializeCriticalSection(&index->lock);
#else
        pthread_mutex_init(&index->lock, NULL);
#endif
    }
}

static void destroy_lock(SylvesSpatialIndex* index) {
    if (index->thread_safe) {
#ifdef _WIN32
        DeleteCriticalSection(&index->lock);
#else
        pthread_mutex_destroy(&index->lock);
#endif
    }
}

static void lock_index(SylvesSpatialIndex* index) {
    if (index->thread_safe) {
#ifdef _WIN32
        EnterCriticalSection(&index->lock);
#else
        pthread_mutex_lock(&index->lock);
#endif
    }
}

static void unlock_index(SylvesSpatialIndex* index) {
    if (index->thread_safe) {
#ifdef _WIN32
        LeaveCriticalSection(&index->lock);
#else
        pthread_mutex_unlock(&index->lock);
#endif
    }
}

static inline int32_t hash_coord(double coord, double inv_cell_size) {
    return (int32_t)floor(coord * inv_cell_size);
}

static inline size_t hash_position(const SylvesVector3* pos, double inv_cell_size, size_t bucket_count) {
    int32_t ix = hash_coord(pos->x, inv_cell_size);
    int32_t iy = hash_coord(pos->y, inv_cell_size);
    int32_t iz = hash_coord(pos->z, inv_cell_size);
    
    /* Simple hash function for spatial coordinates */
    size_t hash = (size_t)ix * 73856093;
    hash ^= (size_t)iy * 19349663;
    hash ^= (size_t)iz * 83492791;
    
    return hash % bucket_count;
}

/* Grid hash implementation */

static GridHashIndex* grid_hash_create(size_t bucket_count, double cell_size) {
    GridHashIndex* hash = (GridHashIndex*)sylves_alloc(sizeof(GridHashIndex));
    if (!hash) {
        return NULL;
    }
    
    hash->buckets = (HashBucket*)sylves_alloc(sizeof(HashBucket) * bucket_count);
    if (!hash->buckets) {
        sylves_free(hash);
        return NULL;
    }
    
    memset(hash->buckets, 0, sizeof(HashBucket) * bucket_count);
    hash->bucket_count = bucket_count;
    hash->cell_size = cell_size;
    hash->inv_cell_size = 1.0 / cell_size;
    hash->item_count = 0;
    
    hash->cell_to_bucket = sylves_hash_create(1024);
    if (!hash->cell_to_bucket) {
        sylves_free(hash->buckets);
        sylves_free(hash);
        return NULL;
    }
    
    return hash;
}

static void grid_hash_destroy(GridHashIndex* hash) {
    if (!hash) {
        return;
    }
    
    /* Free all entries */
    for (size_t i = 0; i < hash->bucket_count; i++) {
        HashEntry* entry = hash->buckets[i].entries;
        while (entry) {
            HashEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    
    sylves_hash_destroy(hash->cell_to_bucket);
    sylves_free(hash->buckets);
    sylves_free(hash);
}

static SylvesError grid_hash_insert(GridHashIndex* hash, const SylvesCell* cell, 
                                   const SylvesVector3* center, void* data) {
    size_t bucket_idx = hash_position(center, hash->inv_cell_size, hash->bucket_count);
    
    /* Create new entry */
    HashEntry* entry = (HashEntry*)sylves_alloc(sizeof(HashEntry));
    if (!entry) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    entry->cell = *cell;
    entry->center = *center;
    entry->data = data;
    entry->next = hash->buckets[bucket_idx].entries;
    
    hash->buckets[bucket_idx].entries = entry;
    hash->buckets[bucket_idx].count++;
    hash->item_count++;
    
    /* Store bucket index for fast removal */
    sylves_hash_set_int(hash->cell_to_bucket, cell, (int)bucket_idx);
    
    return SYLVES_SUCCESS;
}

static SylvesError grid_hash_remove(GridHashIndex* hash, const SylvesCell* cell) {
    int bucket_idx;
    if (!sylves_hash_get_int(hash->cell_to_bucket, cell, &bucket_idx)) {
        return SYLVES_ERROR_NOT_FOUND;
    }
    
    HashBucket* bucket = &hash->buckets[bucket_idx];
    HashEntry** prev_next = &bucket->entries;
    HashEntry* entry = bucket->entries;
    
    while (entry) {
        if (sylves_cell_equals(entry->cell, *cell)) {
            *prev_next = entry->next;
            sylves_free(entry);
            bucket->count--;
            hash->item_count--;
            sylves_hash_remove(hash->cell_to_bucket, cell);
            return SYLVES_SUCCESS;
        }
        prev_next = &entry->next;
        entry = entry->next;
    }
    
    return SYLVES_ERROR_NOT_FOUND;
}

static void grid_hash_query_aabb(const GridHashIndex* hash, SylvesAabb aabb,
                                SylvesCellDataVisitor visitor, void* user_data) {
    /* Calculate hash bounds */
    int32_t min_x = hash_coord(aabb.min.x, hash->inv_cell_size);
    int32_t min_y = hash_coord(aabb.min.y, hash->inv_cell_size);
    int32_t min_z = hash_coord(aabb.min.z, hash->inv_cell_size);
    int32_t max_x = hash_coord(aabb.max.x, hash->inv_cell_size);
    int32_t max_y = hash_coord(aabb.max.y, hash->inv_cell_size);
    int32_t max_z = hash_coord(aabb.max.z, hash->inv_cell_size);
    
    /* Visit all buckets that might contain cells in the AABB */
    for (int32_t x = min_x; x <= max_x; x++) {
        for (int32_t y = min_y; y <= max_y; y++) {
            for (int32_t z = min_z; z <= max_z; z++) {
                SylvesVector3 pos = {
                    x * hash->cell_size,
                    y * hash->cell_size,
                    z * hash->cell_size
                };
                
                size_t bucket_idx = hash_position(&pos, hash->inv_cell_size, hash->bucket_count);
                HashEntry* entry = hash->buckets[bucket_idx].entries;
                
                while (entry) {
                    /* Check if cell center is actually in AABB */
                    if (sylves_aabb_contains_point(aabb, entry->center)) {
                        if (!visitor(&entry->cell, entry->data, user_data)) {
                            return;
                        }
                    }
                    entry = entry->next;
                }
            }
        }
    }
}

/* Public API implementation */

SylvesSpatialIndex* sylves_spatial_index_create(const SylvesSpatialIndexConfig* config, int dimension) {
    if (!config || (dimension != 2 && dimension != 3)) {
        return NULL;
    }
    
    SylvesSpatialIndex* index = (SylvesSpatialIndex*)sylves_alloc(sizeof(SylvesSpatialIndex));
    if (!index) {
        return NULL;
    }
    
    memset(index, 0, sizeof(SylvesSpatialIndex));
    index->type = config->type;
    index->dimension = dimension;
    index->thread_safe = config->thread_safe;
    
    init_lock(index);
    
    /* Create appropriate index type */
    switch (config->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH: {
            size_t bucket_count = config->bucket_size > 0 ? config->bucket_size : 1024;
            double cell_size = 1.0; /* Default, should be configured based on grid */
            
            index->data.grid_hash = grid_hash_create(bucket_count, cell_size);
            if (!index->data.grid_hash) {
                destroy_lock(index);
                sylves_free(index);
                return NULL;
            }
            break;
        }
        
        default:
            /* Other index types not implemented yet */
            destroy_lock(index);
            sylves_free(index);
            return NULL;
    }
    
    return index;
}

void sylves_spatial_index_destroy(SylvesSpatialIndex* index) {
    if (!index) {
        return;
    }
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH:
            grid_hash_destroy(index->data.grid_hash);
            break;
        default:
            break;
    }
    
    destroy_lock(index);
    sylves_free(index);
}

SylvesError sylves_spatial_index_insert(SylvesSpatialIndex* index, const SylvesCell* cell,
                                       const SylvesVector3* center, void* data) {
    if (!index || !cell || !center) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    lock_index(index);
    
    SylvesError result = SYLVES_ERROR_NOT_IMPLEMENTED;
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH:
            result = grid_hash_insert(index->data.grid_hash, cell, center, data);
            break;
        default:
            break;
    }
    
    if (result == SYLVES_SUCCESS) {
        index->stats.item_count++;
    }
    
    unlock_index(index);
    return result;
}

SylvesError sylves_spatial_index_remove(SylvesSpatialIndex* index, const SylvesCell* cell) {
    if (!index || !cell) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    lock_index(index);
    
    SylvesError result = SYLVES_ERROR_NOT_IMPLEMENTED;
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH:
            result = grid_hash_remove(index->data.grid_hash, cell);
            break;
        default:
            break;
    }
    
    if (result == SYLVES_SUCCESS) {
        index->stats.item_count--;
    }
    
    unlock_index(index);
    return result;
}

SylvesError sylves_spatial_index_query_aabb(const SylvesSpatialIndex* index, const SylvesAabb* aabb,
                                           SylvesCellDataVisitor visitor, void* user_data) {
    if (!index || !aabb || !visitor) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH:
            grid_hash_query_aabb(index->data.grid_hash, *aabb, visitor, user_data);
            return SYLVES_SUCCESS;
        default:
            return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
}

SylvesError sylves_spatial_index_query_radius(const SylvesSpatialIndex* index, const SylvesVector3* center,
                                             double radius, SylvesCellDataVisitor visitor, void* user_data) {
    if (!index || !center || radius <= 0 || !visitor) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Convert radius query to AABB query */
    SylvesAabb aabb = {
        .min = { center->x - radius, center->y - radius, center->z - radius },
        .max = { center->x + radius, center->y + radius, center->z + radius }
    };
    
    /* Wrapper to filter by actual radius */
    struct RadiusFilter {
        SylvesVector3 center;
        double radius_sq;
        SylvesCellDataVisitor visitor;
        void* user_data;
    } filter = {
        *center,
        radius * radius,
        visitor,
        user_data
    };
    
    /* TODO: Implement radius filtering wrapper */
    /* For now, just use AABB query without radius filtering */
    return sylves_spatial_index_query_aabb(index, &aabb, visitor, user_data);
}

void sylves_spatial_index_clear(SylvesSpatialIndex* index) {
    if (!index) {
        return;
    }
    
    lock_index(index);
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH: {
            GridHashIndex* hash = index->data.grid_hash;
            
            /* Clear all buckets */
            for (size_t i = 0; i < hash->bucket_count; i++) {
                HashEntry* entry = hash->buckets[i].entries;
                while (entry) {
                    HashEntry* next = entry->next;
                    sylves_free(entry);
                    entry = next;
                }
                hash->buckets[i].entries = NULL;
                hash->buckets[i].count = 0;
            }
            
            hash->item_count = 0;
            sylves_hash_clear(hash->cell_to_bucket);
            break;
        }
        default:
            break;
    }
    
    index->stats.item_count = 0;
    
    unlock_index(index);
}

void sylves_spatial_index_get_stats(const SylvesSpatialIndex* index, SylvesSpatialIndexStats* stats) {
    if (!index || !stats) {
        return;
    }
    
    *stats = index->stats;
    
    switch (index->type) {
        case SYLVES_SPATIAL_INDEX_GRID_HASH: {
            GridHashIndex* hash = index->data.grid_hash;
            stats->item_count = hash->item_count;
            stats->bucket_count = hash->bucket_count;
            
            /* Calculate additional stats */
            size_t non_empty = 0;
            size_t max_bucket_size = 0;
            
            for (size_t i = 0; i < hash->bucket_count; i++) {
                if (hash->buckets[i].count > 0) {
                    non_empty++;
                    if (hash->buckets[i].count > max_bucket_size) {
                        max_bucket_size = hash->buckets[i].count;
                    }
                }
            }
            
            stats->node_count = non_empty;
            stats->empty_nodes = hash->bucket_count - non_empty;
            stats->average_items_per_node = non_empty > 0 ? 
                (double)hash->item_count / non_empty : 0.0;
            break;
        }
        default:
            break;
    }
}

/* Grid spatial hash implementation */

/* Visitor context structures */
struct InsertContext {
    SylvesGridSpatialHash* hash;
    SylvesError last_error;
};

struct SampleContext {
    const SylvesGrid* grid;
    double* total_size;
    size_t* count;
    size_t max_samples;
};

struct BuildContext {
    SylvesSpatialIndex* index;
    const SylvesGrid* grid;
    SylvesError last_error;
};

struct VisitorWrapper {
    SylvesCellVisitor visitor;
    void* user_data;
};

/* Helper function to insert a single cell */
static SylvesError insert_cell(SylvesGridSpatialHash* hash, const SylvesCell* cell) {
    SylvesVector3 center = sylves_grid_get_cell_center(hash->grid, *cell);
    return sylves_spatial_index_insert(hash->index, cell, &center, NULL);
}



/* Wrapper to convert data visitor to cell visitor */
static bool data_visitor(const SylvesCell* cell, void* data, void* wrapper_data) {
    struct VisitorWrapper* w = (struct VisitorWrapper*)wrapper_data;
    return w->visitor(cell, w->user_data);
}

SylvesGridSpatialHash* sylves_grid_spatial_hash_create(const SylvesGrid* grid, double cell_size, bool thread_safe) {
    if (!grid) {
        return NULL;
    }
    
    SylvesGridSpatialHash* hash = (SylvesGridSpatialHash*)sylves_alloc(sizeof(SylvesGridSpatialHash));
    if (!hash) {
        return NULL;
    }
    
    /* Auto-calculate cell size if not provided */
    if (cell_size <= 0) {
        cell_size = sylves_grid_spatial_hash_optimal_size(grid, 100);
    }
    
    SylvesSpatialIndexConfig config = {
        .type = SYLVES_SPATIAL_INDEX_GRID_HASH,
        .bucket_size = 4096,
        .thread_safe = thread_safe
    };
    
    hash->index = sylves_spatial_index_create(&config, sylves_grid_is_3d(grid) ? 3 : 2);
    if (!hash->index) {
        sylves_free(hash);
        return NULL;
    }
    
    /* Update cell size in the index */
    if (hash->index->data.grid_hash) {
        hash->index->data.grid_hash->cell_size = cell_size;
        hash->index->data.grid_hash->inv_cell_size = 1.0 / cell_size;
    }
    
    hash->grid = grid;
    hash->cell_size = cell_size;
    
    return hash;
}

void sylves_grid_spatial_hash_destroy(SylvesGridSpatialHash* hash) {
    if (!hash) {
        return;
    }
    
    sylves_spatial_index_destroy(hash->index);
    sylves_free(hash);
}

SylvesError sylves_grid_spatial_hash_insert_bounds(SylvesGridSpatialHash* hash, const SylvesBound* bounds) {
    if (!hash || !bounds) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Get cell count first */
    int cell_count = sylves_bound_get_cell_count(bounds);
    if (cell_count <= 0) {
        return cell_count == 0 ? SYLVES_SUCCESS : SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Allocate array for cells */
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * cell_count);
    if (!cells) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    /* Get all cells */
    int actual_count = sylves_bound_get_cells(bounds, cells, cell_count);
    if (actual_count < 0) {
        sylves_free(cells);
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Insert each cell */
    SylvesError result = SYLVES_SUCCESS;
    for (int i = 0; i < actual_count && result == SYLVES_SUCCESS; i++) {
        result = insert_cell(hash, &cells[i]);
    }
    
    sylves_free(cells);
    return result;
}

SylvesError sylves_grid_spatial_hash_query_aabb(const SylvesGridSpatialHash* hash,
                                               const SylvesVector3* min, const SylvesVector3* max,
                                               SylvesCellVisitor visitor, void* user_data) {
    if (!hash || !min || !max || !visitor) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    SylvesAabb aabb = { *min, *max };
    
    /* Wrapper to convert data visitor to cell visitor */
    struct VisitorWrapper wrapper = { visitor, user_data };
    
    return sylves_spatial_index_query_aabb(hash->index, &aabb, data_visitor, &wrapper);
}

double sylves_grid_spatial_hash_optimal_size(const SylvesGrid* grid, size_t sample_count) {
    if (!grid || sample_count == 0) {
        return 1.0;
    }
    
    /* Get grid bounds */
    const SylvesBound* bounds = sylves_grid_get_bound(grid);
    if (!bounds) {
        return 1.0;
    }
    
    /* Get cell count */
    int cell_count = sylves_bound_get_cell_count(bounds);
    if (cell_count <= 0) {
        return 1.0;
    }
    
    /* Limit sample count */
    size_t actual_sample_count = sample_count;
    if (actual_sample_count > (size_t)cell_count) {
        actual_sample_count = (size_t)cell_count;
    }
    
    /* Allocate array for sample cells */
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * actual_sample_count);
    if (!cells) {
        return 1.0;
    }
    
    /* Get sample cells */
    int got_cells = sylves_bound_get_cells(bounds, cells, actual_sample_count);
    if (got_cells <= 0) {
        sylves_free(cells);
        return 1.0;
    }
    
    /* Sample cell sizes */
    double total_size = 0.0;
    size_t actual_samples = 0;
    
    for (int i = 0; i < got_cells && actual_samples < sample_count; i++) {
        SylvesAabb aabb;
        if (sylves_grid_get_cell_aabb(grid, cells[i], &aabb) == SYLVES_SUCCESS) {
            double size = fmax(fmax(aabb.max.x - aabb.min.x,
                                   aabb.max.y - aabb.min.y),
                              aabb.max.z - aabb.min.z);
            total_size += size;
            actual_samples++;
        }
    }
    
    sylves_free(cells);
    
    if (actual_samples == 0) {
        return 1.0;
    }
    
    /* Use average cell size * 2 as hash cell size */
    return (total_size / actual_samples) * 2.0;
}

SylvesError sylves_spatial_index_build_from_grid(SylvesSpatialIndex* index, const SylvesGrid* grid,
                                                const SylvesBound* bounds) {
    if (!index || !grid) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Use provided bounds or get from grid */
    const SylvesBound* grid_bounds = NULL;
    bool should_free_bounds = false;
    if (!bounds) {
        grid_bounds = sylves_grid_get_bound(grid);
        if (!grid_bounds) {
            return SYLVES_ERROR_INVALID_ARGUMENT;
        }
        bounds = grid_bounds;
    }
    
    /* Get cell count */
    int cell_count = sylves_bound_get_cell_count(bounds);
    if (cell_count <= 0) {
        return cell_count == 0 ? SYLVES_SUCCESS : SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Allocate array for cells */
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * cell_count);
    if (!cells) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    /* Get all cells */
    int actual_count = sylves_bound_get_cells(bounds, cells, cell_count);
    if (actual_count < 0) {
        sylves_free(cells);
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Insert each cell */
    SylvesError result = SYLVES_SUCCESS;
    for (int i = 0; i < actual_count && result == SYLVES_SUCCESS; i++) {
        SylvesVector3 center = sylves_grid_get_cell_center(grid, cells[i]);
        result = sylves_spatial_index_insert(index, &cells[i], &center, NULL);
    }
    
    sylves_free(cells);
    return result;
}
