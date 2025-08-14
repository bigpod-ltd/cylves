#ifndef SYLVES_SPATIAL_INDEX_H
#define SYLVES_SPATIAL_INDEX_H

#include "sylves/sylves.h"
#include "sylves/aabb.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup spatial_index Spatial Indexing System
 * @brief Efficient spatial indexing for range queries and cell lookups
 * @{
 */

/**
 * Spatial index types
 */
typedef enum SylvesSpatialIndexType {
    SYLVES_SPATIAL_INDEX_GRID_HASH,    /**< Grid-based spatial hashing */
    SYLVES_SPATIAL_INDEX_QUADTREE,     /**< Quadtree for 2D grids */
    SYLVES_SPATIAL_INDEX_OCTREE,       /**< Octree for 3D grids */
    SYLVES_SPATIAL_INDEX_RTREE,        /**< R-tree for arbitrary dimensions */
    SYLVES_SPATIAL_INDEX_KDTREE        /**< K-d tree */
} SylvesSpatialIndexType;

/**
 * Spatial index configuration
 */
typedef struct SylvesSpatialIndexConfig {
    SylvesSpatialIndexType type;       /**< Index type */
    size_t bucket_size;                /**< Bucket size for hash grid */
    size_t max_depth;                  /**< Maximum tree depth */
    size_t max_items_per_node;         /**< Max items before split */
    bool dynamic_rebalance;            /**< Enable dynamic rebalancing */
    bool thread_safe;                  /**< Enable thread safety */
} SylvesSpatialIndexConfig;

/**
 * Spatial index statistics
 */
typedef struct SylvesSpatialIndexStats {
    size_t item_count;                 /**< Number of indexed items */
    size_t node_count;                 /**< Number of nodes (for trees) */
    size_t bucket_count;               /**< Number of buckets (for hash) */
    size_t max_depth;                  /**< Current maximum depth */
    double average_items_per_node;     /**< Average items per node */
    size_t empty_nodes;                /**< Number of empty nodes */
    size_t query_count;                /**< Total queries performed */
    double average_query_time_ms;      /**< Average query time */
} SylvesSpatialIndexStats;

/**
 * Opaque spatial index structure
 */
typedef struct SylvesSpatialIndex SylvesSpatialIndex;

/**
 * Grid-specific spatial hash
 */
typedef struct SylvesGridSpatialHash SylvesGridSpatialHash;

/**
 * Cell visitor callback
 * @param cell Cell being visited
 * @param user_data User-provided context
 * @return true to continue iteration, false to stop
 */
typedef bool (*SylvesCellVisitor)(const SylvesCell* cell, void* user_data);

/**
 * Cell with data visitor callback
 * @param cell Cell being visited
 * @param data Associated data
 * @param user_data User-provided context
 * @return true to continue iteration, false to stop
 */
typedef bool (*SylvesCellDataVisitor)(const SylvesCell* cell, void* data, void* user_data);

/* General spatial index functions */

/**
 * Create a spatial index with configuration
 * @param config Index configuration
 * @param dimension Spatial dimension (2 or 3)
 * @return New spatial index or NULL on failure
 */
SYLVES_EXPORT SylvesSpatialIndex* sylves_spatial_index_create(
    const SylvesSpatialIndexConfig* config,
    int dimension
);

/**
 * Destroy spatial index
 * @param index Index to destroy
 */
SYLVES_EXPORT void sylves_spatial_index_destroy(SylvesSpatialIndex* index);

/**
 * Insert cell into spatial index
 * @param index Spatial index
 * @param cell Cell to insert
 * @param center Cell center position
 * @param data Optional associated data
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_insert(
    SylvesSpatialIndex* index,
    const SylvesCell* cell,
    const SylvesVector3* center,
    void* data
);

/**
 * Remove cell from spatial index
 * @param index Spatial index
 * @param cell Cell to remove
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_remove(
    SylvesSpatialIndex* index,
    const SylvesCell* cell
);

/**
 * Query cells within AABB
 * @param index Spatial index
 * @param aabb Query bounds
 * @param visitor Callback for each cell found
 * @param user_data User context for callback
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_query_aabb(
    const SylvesSpatialIndex* index,
    const SylvesAabb* aabb,
    SylvesCellDataVisitor visitor,
    void* user_data
);

/**
 * Query cells within radius
 * @param index Spatial index
 * @param center Query center
 * @param radius Query radius
 * @param visitor Callback for each cell found
 * @param user_data User context for callback
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_query_radius(
    const SylvesSpatialIndex* index,
    const SylvesVector3* center,
    double radius,
    SylvesCellDataVisitor visitor,
    void* user_data
);

/**
 * Find nearest cell to point
 * @param index Spatial index
 * @param point Query point
 * @param out_cell Output nearest cell
 * @param out_distance Output distance to cell
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_find_nearest(
    const SylvesSpatialIndex* index,
    const SylvesVector3* point,
    SylvesCell* out_cell,
    double* out_distance
);

/**
 * Clear all cells from index
 * @param index Spatial index
 */
SYLVES_EXPORT void sylves_spatial_index_clear(SylvesSpatialIndex* index);

/**
 * Get index statistics
 * @param index Spatial index
 * @param stats Output statistics
 */
SYLVES_EXPORT void sylves_spatial_index_get_stats(
    const SylvesSpatialIndex* index,
    SylvesSpatialIndexStats* stats
);

/**
 * Optimize index structure
 * @param index Spatial index
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_optimize(SylvesSpatialIndex* index);

/* Grid-specific spatial hash */

/**
 * Create grid-optimized spatial hash
 * @param grid Grid to index
 * @param cell_size Hash cell size (0 for auto)
 * @param thread_safe Enable thread safety
 * @return New spatial hash or NULL on failure
 */
SYLVES_EXPORT SylvesGridSpatialHash* sylves_grid_spatial_hash_create(
    const SylvesGrid* grid,
    double cell_size,
    bool thread_safe
);

/**
 * Destroy grid spatial hash
 * @param hash Hash to destroy
 */
SYLVES_EXPORT void sylves_grid_spatial_hash_destroy(SylvesGridSpatialHash* hash);

/**
 * Insert cells from grid bounds
 * @param hash Spatial hash
 * @param bounds Grid bounds to index
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_grid_spatial_hash_insert_bounds(
    SylvesGridSpatialHash* hash,
    const SylvesBound* bounds
);

/**
 * Query cells in AABB using grid acceleration
 * @param hash Spatial hash
 * @param min AABB minimum
 * @param max AABB maximum
 * @param visitor Callback for each cell
 * @param user_data User context
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_grid_spatial_hash_query_aabb(
    const SylvesGridSpatialHash* hash,
    const SylvesVector3* min,
    const SylvesVector3* max,
    SylvesCellVisitor visitor,
    void* user_data
);

/**
 * Raycast through spatial hash
 * @param hash Spatial hash
 * @param origin Ray origin
 * @param direction Ray direction
 * @param max_distance Maximum distance
 * @param visitor Callback for each cell hit
 * @param user_data User context
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_grid_spatial_hash_raycast(
    const SylvesGridSpatialHash* hash,
    const SylvesVector3* origin,
    const SylvesVector3* direction,
    double max_distance,
    SylvesCellVisitor visitor,
    void* user_data
);

/**
 * Get optimal hash cell size for grid
 * @param grid Grid to analyze
 * @param sample_count Number of cells to sample
 * @return Recommended hash cell size
 */
SYLVES_EXPORT double sylves_grid_spatial_hash_optimal_size(
    const SylvesGrid* grid,
    size_t sample_count
);

/* Batch operations */

/**
 * Insert multiple cells efficiently
 * @param index Spatial index
 * @param cells Array of cells
 * @param centers Array of cell centers
 * @param count Number of cells
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_insert_batch(
    SylvesSpatialIndex* index,
    const SylvesCell* cells,
    const SylvesVector3* centers,
    size_t count
);

/**
 * Build spatial index from grid
 * @param index Spatial index
 * @param grid Grid to index
 * @param bounds Optional bounds to limit indexing
 * @return SYLVES_SUCCESS or error code
 */
SYLVES_EXPORT SylvesError sylves_spatial_index_build_from_grid(
    SylvesSpatialIndex* index,
    const SylvesGrid* grid,
    const SylvesBound* bounds
);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_SPATIAL_INDEX_H */
