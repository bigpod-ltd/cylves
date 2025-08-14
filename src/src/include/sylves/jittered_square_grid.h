/**
 * @file jittered_square_grid.h
 * @brief Jittered square grid implementation
 */

#ifndef SYLVES_JITTERED_SQUARE_GRID_H
#define SYLVES_JITTERED_SQUARE_GRID_H

#include "sylves/grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Options for jittered square grid generation
 */
typedef struct {
    /** @brief Random seed for jittering (0 for random) */
    unsigned int seed;
    
    /** @brief Maximum jitter amount (0.0 to 0.5, as fraction of cell size) */
    double jitter_amount;
    
    /** @brief Number of cells to generate in each direction */
    int grid_size;
} SylvesJitteredSquareOptions;

/**
 * @brief Create a jittered square grid
 * 
 * Creates a square grid where each vertex is randomly perturbed
 * from its regular position, then generates a Voronoi diagram.
 * 
 * @param cell_size Size of each cell in the regular grid
 * @param options Configuration options (can be NULL for defaults)
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_jittered_square_grid_create(
    double cell_size,
    const SylvesJitteredSquareOptions* options
);

/**
 * @brief Create default jittered square grid options
 * @return Default options structure
 */
SylvesJitteredSquareOptions sylves_jittered_square_options_default(void);

/**
 * @brief Create a perturbed grid from any base grid
 * 
 * Takes an existing grid and applies random perturbations to vertices.
 * 
 * @param base_grid The grid to perturb
 * @param perturbation_amount Maximum perturbation distance
 * @param seed Random seed (0 for random)
 * @return Pointer to the perturbed grid, or NULL on failure
 */
SylvesGrid* sylves_perturbed_grid_create(
    const SylvesGrid* base_grid,
    double perturbation_amount,
    unsigned int seed
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_JITTERED_SQUARE_GRID_H */
