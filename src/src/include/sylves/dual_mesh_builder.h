/**
 * @file dual_mesh_builder.h
 * @brief Dual mesh builder for creating dual meshes from primal meshes
 */

#ifndef SYLVES_DUAL_MESH_BUILDER_H
#define SYLVES_DUAL_MESH_BUILDER_H

#include "types.h"
#include "mesh_data.h"
#include "errors.h"

/* Forward declarations */
typedef struct SylvesDualMeshBuilder SylvesDualMeshBuilder;

typedef struct SylvesDualMapping {
    int primal_face;
    int primal_vertex;
    int dual_face;
    int dual_vertex;
} SylvesDualMapping;

/**
 * @brief Dual mesh builder configuration
 * 
 * Configuration options for dual mesh generation
 */
typedef struct SylvesDualMeshConfig {
    bool include_boundary_faces;  /**< Include boundary faces in dual mesh */
    bool center_on_centroid;      /**< Use face centroid instead of incenter */
    double shrink_factor;         /**< Shrink factor for dual vertices (0.0-1.0) */
} SylvesDualMeshConfig;

/**
 * @brief Create default dual mesh configuration
 * 
 * @return Default configuration with:
 *         - include_boundary_faces = false
 *         - center_on_centroid = true
 *         - shrink_factor = 1.0
 */
SylvesDualMeshConfig sylves_dual_mesh_config_default(void);

/**
 * @brief Build dual mesh from primal mesh
 * 
 * Creates a dual mesh where:
 * - Each face of the primal mesh becomes a vertex in the dual mesh
 * - Each edge of the primal mesh becomes an edge in the dual mesh
 * - Each vertex of the primal mesh becomes a face in the dual mesh
 * 
 * @param primal The primal mesh to dualize
 * @param config Configuration options (NULL for default)
 * @return New dual mesh or NULL on error
 */
SylvesMeshDataEx* sylves_dual_mesh_build(
    const SylvesMeshDataEx* primal,
    const SylvesDualMeshConfig* config);

/**
 * @brief Build dual mesh from simple mesh data
 * 
 * Convenience function that converts simple mesh to extended format,
 * builds dual, and returns extended format result.
 * 
 * @param primal Simple mesh data
 * @param config Configuration options (NULL for default)
 * @return New dual mesh or NULL on error
 */
SylvesMeshDataEx* sylves_dual_mesh_build_simple(
    const SylvesMeshData* primal,
    const SylvesDualMeshConfig* config);

/**
 * @brief Get dual vertex position for a primal face
 * 
 * Calculates the position of a dual vertex corresponding to a primal face.
 * This can be the face centroid, incenter, or other representative point.
 * 
 * @param primal The primal mesh
 * @param face_index Index of the face in the primal mesh
 * @param config Configuration options affecting vertex placement
 * @param[out] position Output position of the dual vertex
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_dual_mesh_get_vertex_position(
    const SylvesMeshDataEx* primal,
    int face_index,
    const SylvesDualMeshConfig* config,
    SylvesVector3* position);

/**
 * @brief Validate dual mesh topology
 * 
 * Checks that the dual mesh has valid topology with respect to the primal mesh.
 * 
 * @param dual The dual mesh to validate
 * @param primal The original primal mesh
 * @return true if topology is valid, false otherwise
 */
bool sylves_dual_mesh_validate_topology(
    const SylvesMeshDataEx* dual,
    const SylvesMeshDataEx* primal);

#endif /* SYLVES_DUAL_MESH_BUILDER_H */
