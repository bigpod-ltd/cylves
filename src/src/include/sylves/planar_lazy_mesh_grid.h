/**
 * @file planar_lazy_mesh_grid.h
 * @brief Planar lazy mesh grid - infinite planar grid evaluated lazily by chunks
 */

#ifndef SYLVES_PLANAR_LAZY_MESH_GRID_H
#define SYLVES_PLANAR_LAZY_MESH_GRID_H

#include "sylves/grid.h"
#include "sylves/mesh_data.h"
#include "sylves/bounds.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for generating mesh data for a chunk
 * 
 * @param chunk_x X coordinate of the chunk
 * @param chunk_y Y coordinate of the chunk
 * @param user_data User-provided data
 * @return Mesh data for the chunk, or NULL on error. The caller takes ownership.
 */
typedef SylvesMeshData* (*SylvesGetMeshDataFunc)(int chunk_x, int chunk_y, void* user_data);

/**
 * @brief Options for mesh grid generation
 */
typedef struct {
    /** Whether to validate mesh data */
    bool validate_mesh;
    
    /** Whether to compute adjacency automatically */
    bool compute_adjacency;
    
    /** Whether to allow non-manifold meshes */
    bool allow_non_manifold;
    
    /** Maximum vertices per face (0 for unlimited) */
    int max_vertices_per_face;
} SylvesMeshGridOptions;

/**
 * @brief Cache policy for storing generated chunks
 */
typedef enum {
    SYLVES_CACHE_NONE = 0,      /**< Don't cache chunks */
    SYLVES_CACHE_LRU = 1,        /**< LRU cache with limited size */
    SYLVES_CACHE_ALWAYS = 2,     /**< Cache all chunks (unbounded) */
} SylvesCachePolicy;

/**
 * @brief Create a planar lazy mesh grid with rectangular chunks
 * 
 * @param get_mesh_data Function to generate mesh data for each chunk
 * @param stride_x X stride between chunks
 * @param stride_y Y stride between chunks
 * @param aabb_min Minimum corner of chunk bounding box
 * @param aabb_max Maximum corner of chunk bounding box
 * @param translate_mesh_data Whether to translate mesh data by chunk offset
 * @param options Mesh grid options (can be NULL for defaults)
 * @param bound Optional bound to limit chunk generation
 * @param cache_policy Cache policy for chunks
 * @param user_data User data passed to get_mesh_data
 * @return New planar lazy mesh grid, or NULL on error
 */
SylvesGrid* sylves_planar_lazy_mesh_grid_create(
    SylvesGetMeshDataFunc get_mesh_data,
    SylvesVector2 stride_x,
    SylvesVector2 stride_y,
    SylvesVector2 aabb_min,
    SylvesVector2 aabb_max,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data
);

/**
 * @brief Create a planar lazy mesh grid with square grid chunks
 * 
 * @param get_mesh_data Function to generate mesh data for each chunk
 * @param chunk_size Size of each square chunk
 * @param margin Extra margin around each chunk for mesh data
 * @param translate_mesh_data Whether to translate mesh data by chunk offset
 * @param options Mesh grid options (can be NULL for defaults)
 * @param bound Optional bound to limit chunk generation
 * @param cache_policy Cache policy for chunks
 * @param user_data User data passed to get_mesh_data
 * @return New planar lazy mesh grid, or NULL on error
 */
SylvesGrid* sylves_planar_lazy_mesh_grid_create_square(
    SylvesGetMeshDataFunc get_mesh_data,
    double chunk_size,
    double margin,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data
);

/**
 * @brief Create a planar lazy mesh grid with hex grid chunks
 * 
 * @param get_mesh_data Function to generate mesh data for each chunk
 * @param hex_size Size of each hexagonal chunk
 * @param margin Extra margin around each chunk for mesh data
 * @param translate_mesh_data Whether to translate mesh data by chunk offset
 * @param options Mesh grid options (can be NULL for defaults)
 * @param bound Optional bound to limit chunk generation
 * @param cache_policy Cache policy for chunks
 * @param user_data User data passed to get_mesh_data
 * @return New planar lazy mesh grid, or NULL on error
 */
SylvesGrid* sylves_planar_lazy_mesh_grid_create_hex(
    SylvesGetMeshDataFunc get_mesh_data,
    double hex_size,
    double margin,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data
);

/**
 * @brief Initialize default mesh grid options
 */
void sylves_mesh_grid_options_init(SylvesMeshGridOptions* options);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_PLANAR_LAZY_MESH_GRID_H */
