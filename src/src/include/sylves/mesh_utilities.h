/**
 * @file mesh_utilities.h
 * @brief Mesh utility functions for merging, splitting, optimization, and repair
 */

#ifndef SYLVES_MESH_UTILITIES_H
#define SYLVES_MESH_UTILITIES_H

#include "types.h"
#include "mesh_data.h"
#include "errors.h"

/**
 * @brief Configuration for mesh merging operations
 */
typedef struct SylvesMeshMergeConfig {
    double vertex_merge_distance;   /**< Distance threshold for merging vertices */
    bool merge_attributes;          /**< Whether to merge normals, UVs, etc. */
    bool remove_duplicates;         /**< Remove duplicate faces */
} SylvesMeshMergeConfig;

/**
 * @brief Configuration for mesh optimization
 */
typedef struct SylvesMeshOptimizeConfig {
    bool remove_unused_vertices;    /**< Remove vertices not referenced by any face */
    bool merge_coplanar_faces;      /**< Merge adjacent coplanar faces */
    double coplanar_angle_tolerance;/**< Angle tolerance for coplanar test (radians) */
} SylvesMeshOptimizeConfig;

/**
 * @brief Configuration for UV generation
 */
typedef struct SylvesUVGenerationConfig {
    enum {
        SYLVES_UV_PLANAR_PROJECTION,
        SYLVES_UV_SPHERICAL_MAPPING,
        SYLVES_UV_CYLINDRICAL_MAPPING,
        SYLVES_UV_BOX_MAPPING
    } mapping_type;
    SylvesVector3 projection_axis;  /**< Axis for planar/cylindrical projection */
    double scale;                   /**< UV coordinate scale factor */
} SylvesUVGenerationConfig;

/* Mesh merging */

/**
 * @brief Merge multiple meshes into one
 * 
 * @param meshes Array of meshes to merge
 * @param mesh_count Number of meshes
 * @param config Merge configuration (NULL for defaults)
 * @return New merged mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_merge(
    const SylvesMeshDataEx** meshes,
    size_t mesh_count,
    const SylvesMeshMergeConfig* config);

/**
 * @brief Merge two meshes
 * 
 * @param mesh1 First mesh
 * @param mesh2 Second mesh
 * @param config Merge configuration (NULL for defaults)
 * @return New merged mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_merge_pair(
    const SylvesMeshDataEx* mesh1,
    const SylvesMeshDataEx* mesh2,
    const SylvesMeshMergeConfig* config);

/* Mesh splitting */

/**
 * @brief Split mesh by connectivity
 * 
 * Separates mesh into connected components.
 * 
 * @param mesh Mesh to split
 * @param[out] components Array to receive component meshes
 * @param[out] component_count Number of components found
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_split_connected(
    const SylvesMeshDataEx* mesh,
    SylvesMeshDataEx*** components,
    size_t* component_count);

/**
 * @brief Split mesh by submesh
 * 
 * Creates separate meshes for each submesh.
 * 
 * @param mesh Mesh to split
 * @param[out] submeshes Array to receive submesh meshes
 * @param[out] submesh_count Number of submeshes
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_split_submeshes(
    const SylvesMeshDataEx* mesh,
    SylvesMeshDataEx*** submeshes,
    size_t* submesh_count);

/* Mesh optimization */

/**
 * @brief Optimize mesh structure
 * 
 * Performs various optimizations like removing unused vertices,
 * merging coplanar faces, etc.
 * 
 * @param mesh Mesh to optimize
 * @param config Optimization configuration (NULL for defaults)
 * @return New optimized mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_optimize(
    const SylvesMeshDataEx* mesh,
    const SylvesMeshOptimizeConfig* config);

/**
 * @brief Remove duplicate vertices
 * 
 * @param mesh Mesh to process
 * @param merge_distance Distance threshold for considering vertices duplicate
 * @return New mesh with duplicates removed or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_remove_duplicate_vertices(
    const SylvesMeshDataEx* mesh,
    double merge_distance);

/* Normal operations */

/**
 * @brief Smooth mesh normals
 * 
 * Applies smoothing to vertex normals based on adjacent face normals.
 * 
 * @param mesh Mesh to smooth normals for
 * @param iterations Number of smoothing iterations
 * @param factor Smoothing factor (0.0-1.0)
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_smooth_normals(
    SylvesMeshDataEx* mesh,
    int iterations,
    double factor);

/**
 * @brief Flip normals
 * 
 * Reverses the direction of all normals in the mesh.
 * 
 * @param mesh Mesh to flip normals for
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_flip_normals(SylvesMeshDataEx* mesh);

/* UV generation */

/**
 * @brief Generate UV coordinates
 * 
 * Creates UV coordinates for the mesh using the specified mapping method.
 * 
 * @param mesh Mesh to generate UVs for
 * @param config UV generation configuration
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_generate_uvs(
    SylvesMeshDataEx* mesh,
    const SylvesUVGenerationConfig* config);

/**
 * @brief Generate planar UV mapping
 * 
 * @param mesh Mesh to generate UVs for
 * @param axis Projection axis
 * @param scale UV scale factor
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_generate_planar_uvs(
    SylvesMeshDataEx* mesh,
    SylvesVector3 axis,
    double scale);

/* Mesh validation and repair */

/**
 * @brief Check for non-manifold edges
 * 
 * @param mesh Mesh to check
 * @param[out] edge_count Number of non-manifold edges found
 * @return true if mesh has non-manifold edges
 */
bool sylves_mesh_has_non_manifold_edges(
    const SylvesMeshDataEx* mesh,
    size_t* edge_count);

/**
 * @brief Check for degenerate faces
 * 
 * @param mesh Mesh to check
 * @param[out] face_count Number of degenerate faces found
 * @return true if mesh has degenerate faces
 */
bool sylves_mesh_has_degenerate_faces(
    const SylvesMeshDataEx* mesh,
    size_t* face_count);

/**
 * @brief Repair mesh
 * 
 * Attempts to fix common mesh issues like non-manifold edges,
 * degenerate faces, inconsistent winding, etc.
 * 
 * @param mesh Mesh to repair
 * @return New repaired mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_repair(const SylvesMeshDataEx* mesh);

/**
 * @brief Remove degenerate faces
 * 
 * @param mesh Mesh to process
 * @return New mesh with degenerate faces removed or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_remove_degenerate_faces(const SylvesMeshDataEx* mesh);

/* Default configurations */

/**
 * @brief Get default merge configuration
 */
SylvesMeshMergeConfig sylves_mesh_merge_config_default(void);

/**
 * @brief Get default optimization configuration
 */
SylvesMeshOptimizeConfig sylves_mesh_optimize_config_default(void);

/**
 * @brief Get default UV generation configuration
 */
SylvesUVGenerationConfig sylves_uv_generation_config_default(void);

/* Face utilities */

/**
 * @brief Get the number of vertices in a face of a submesh
 * 
 * Iterates faces using the face iterator to find the given face index
 * and returns its vertex count.
 *
 * @param mesh Mesh to query
 * @param submesh Submesh index
 * @param face_index Face index within submesh (0-based)
 * @param[out] count_out Vertex count of the face
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_utils_get_face_vertex_count(
    const SylvesMeshDataEx* mesh,
    int submesh,
    int face_index,
    int* count_out);

#endif /* SYLVES_MESH_UTILITIES_H */
