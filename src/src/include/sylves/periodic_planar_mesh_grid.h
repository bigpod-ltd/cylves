/**
 * @file periodic_planar_mesh_grid.h
 * @brief Periodic planar mesh grid implementation
 */

#ifndef SYLVES_PERIODIC_PLANAR_MESH_GRID_H
#define SYLVES_PERIODIC_PLANAR_MESH_GRID_H

#include "sylves/grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Types of periodic planar tilings
 */
typedef enum {
    SYLVES_PERIODIC_CAIRO,          /**< Cairo pentagonal tiling */
    SYLVES_PERIODIC_RHOMBILLE,      /**< Rhombille tiling (rhombic tiling) */
    SYLVES_PERIODIC_TRIHEX,         /**< Trihexagonal (3-6-3-6) tiling */
    SYLVES_PERIODIC_TETRAKIS_SQUARE, /**< Tetrakis square tiling */
    SYLVES_PERIODIC_SQUARE_SNUB     /**< Snub square tiling */
} SylvesPeriodicTilingType;

/**
 * @brief Create a Cairo pentagonal tiling grid
 * 
 * Creates a tiling of irregular pentagons arranged in a periodic pattern.
 * 
 * @param period_x Period in X direction
 * @param period_y Period in Y direction
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_cairo_grid_create(double period_x, double period_y);

/**
 * @brief Create a Rhombille tiling grid
 * 
 * Creates a tiling of rhombs (60-120 degree rhombi).
 * 
 * @param period_x Period in X direction
 * @param period_y Period in Y direction
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_rhombille_grid_create(double period_x, double period_y);

/**
 * @brief Create a Trihexagonal tiling grid
 * 
 * Creates a tiling with alternating triangles and hexagons.
 * 
 * @param period_x Period in X direction
 * @param period_y Period in Y direction
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_trihex_grid_create(double period_x, double period_y);

/**
 * @brief Create a generic periodic planar mesh grid
 * 
 * @param type Type of periodic tiling
 * @param period_x Period in X direction
 * @param period_y Period in Y direction
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_periodic_planar_mesh_grid_create(
    SylvesPeriodicTilingType type,
    double period_x,
    double period_y
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_PERIODIC_PLANAR_MESH_GRID_H */
