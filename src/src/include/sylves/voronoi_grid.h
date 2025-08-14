/**
 * @file voronoi_grid.h
 * @brief Voronoi grid implementation
 */

#ifndef SYLVES_VORONOI_GRID_H
#define SYLVES_VORONOI_GRID_H

#include "sylves/grid.h"
#include "sylves/mesh_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Options for Voronoi grid generation
 */
typedef struct {
    /** @brief Minimum bounds for clipping (NULL for no clipping) */
    SylvesVector2* clip_min;
    
    /** @brief Maximum bounds for clipping (NULL for no clipping) */
    SylvesVector2* clip_max;
    
    /** @brief Number of Lloyd relaxation iterations to apply */
    int lloyd_relaxation_iterations;
    
    /** @brief Whether to pin border points during relaxation */
    bool pin_border_during_relaxation;
} SylvesVoronoiGridOptions;

/**
 * @brief Create a Voronoi grid from a set of points
 * 
 * The Voronoi grid creates cells where each cell contains all points
 * closer to its generating point than to any other generating point.
 * 
 * @param points Array of 2D points that generate the Voronoi cells
 * @param num_points Number of points
 * @param options Optional configuration (can be NULL for defaults)
 * @return Pointer to the created Voronoi grid, or NULL on failure
 */
SylvesGrid* sylves_voronoi_grid_create(
    const SylvesVector2* points, 
    size_t num_points,
    const SylvesVoronoiGridOptions* options
);

/**
 * @brief Create default Voronoi grid options
 * @return Default options structure
 */
SylvesVoronoiGridOptions sylves_voronoi_grid_options_default(void);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_VORONOI_GRID_H */
