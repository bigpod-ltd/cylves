/**
 * @file relaxation_modifier.h
 * @brief Relaxation modifier for smoothing grid vertex positions
 * 
 * The relaxation modifier applies iterative vertex smoothing to grids,
 * producing more regular and evenly-distributed cell shapes. This is
 * particularly useful for mesh grids and Voronoi diagrams.
 * 
 * The modifier supports several relaxation algorithms:
 * - Laplacian smoothing: moves vertices toward the average of their neighbors
 * - Lloyd relaxation: moves vertices to the centroid of their Voronoi cells
 * - Area-weighted smoothing: weights neighbor contributions by face areas
 * 
 * Relaxation parameters:
 * - Iteration count: number of smoothing passes
 * - Relaxation factor: blend between original and relaxed positions (0-1)
 * - Boundary constraints: whether to fix boundary vertices
 * 
 * Example usage:
 * @code
 * // Create a mesh grid
 * SylvesGrid* mesh = sylves_mesh_grid_create(mesh_data);
 * 
 * // Apply 5 iterations of Laplacian smoothing with 0.5 relaxation factor
 * SylvesRelaxationOptions options = {
 *     .algorithm = SYLVES_RELAXATION_LAPLACIAN,
 *     .iterations = 5,
 *     .factor = 0.5,
 *     .fix_boundary = true
 * };
 * SylvesGrid* smoothed = sylves_relaxation_modifier_create(mesh, &options);
 * @endcode
 */

#ifndef SYLVES_RELAXATION_MODIFIER_H
#define SYLVES_RELAXATION_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Relaxation algorithm types
 */
typedef enum {
    /** Laplacian smoothing - moves vertices toward neighbor average */
    SYLVES_RELAXATION_LAPLACIAN,
    
    /** Lloyd relaxation - moves vertices to Voronoi cell centroids */
    SYLVES_RELAXATION_LLOYD,
    
    /** Area-weighted smoothing - weights neighbors by face areas */
    SYLVES_RELAXATION_AREA_WEIGHTED
} SylvesRelaxationAlgorithm;

/**
 * @brief Options for relaxation modifier
 */
typedef struct {
    /** The relaxation algorithm to use */
    SylvesRelaxationAlgorithm algorithm;
    
    /** Number of relaxation iterations to perform */
    int iterations;
    
    /** Relaxation factor (0-1), where 0 = no change, 1 = full relaxation */
    double factor;
    
    /** Whether to fix boundary vertices in place */
    bool fix_boundary;
    
    /** Optional custom weight function for neighbor contributions */
    double (*weight_func)(const SylvesGrid* grid, SylvesCell from, SylvesCell to);
} SylvesRelaxationOptions;

/**
 * @brief Creates default relaxation options
 * 
 * @return Default options with Laplacian smoothing, 1 iteration, factor 0.5
 */
SylvesRelaxationOptions sylves_relaxation_options_default(void);

/**
 * @brief Creates a relaxation modifier that smooths vertex positions
 * 
 * The relaxation modifier applies iterative smoothing to grid vertices,
 * creating more regular cell shapes. The original grid topology is preserved.
 * 
 * @param base_grid The underlying grid to relax (must support vertex queries)
 * @param options Relaxation options, or NULL for defaults
 * @return New relaxation modifier grid, or NULL on error
 * 
 * @note The base grid must support get_cell_center operations
 * @note The created grid takes ownership of the base grid
 */
SylvesGrid* sylves_relaxation_modifier_create(
    SylvesGrid* base_grid,
    const SylvesRelaxationOptions* options);

/**
 * @brief Gets the relaxed position of a cell center
 * 
 * Returns the position of a cell center after relaxation has been applied.
 * 
 * @param grid The relaxation modifier grid
 * @param cell The cell to query
 * @return The relaxed cell center position
 */
SylvesVector3 sylves_relaxation_modifier_get_relaxed_center(
    const SylvesGrid* grid,
    SylvesCell cell);

/**
 * @brief Performs one iteration of relaxation on a set of positions
 * 
 * This utility function applies one relaxation iteration to an array of
 * positions, useful for custom relaxation implementations.
 * 
 * @param positions Array of vertex positions to relax
 * @param num_positions Number of positions
 * @param neighbors Array of neighbor indices for each position
 * @param num_neighbors Array of neighbor counts for each position
 * @param algorithm The relaxation algorithm to use
 * @param factor The relaxation factor (0-1)
 * @param fixed Array of flags indicating fixed vertices (can be NULL)
 */
void sylves_relaxation_iterate(
    SylvesVector3* positions,
    size_t num_positions,
    const int* neighbors,
    const int* num_neighbors,
    SylvesRelaxationAlgorithm algorithm,
    double factor,
    const bool* fixed);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_RELAXATION_MODIFIER_H */
