/**
 * @file grid_defaults.h
 * @brief Default implementations and property helpers for grids
 */

#ifndef GRID_DEFAULTS_H
#define GRID_DEFAULTS_H

#include "sylves/types.h"
#include "grid_internal.h"


/* Property query helpers with fallback logic */

/**
 * @brief Get property with fallback - is_2d
 * This checks cell types if vtable method is NULL
 */
bool sylves_grid_default_is_2d(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - is_3d
 * This checks cell types if vtable method is NULL
 */
bool sylves_grid_default_is_3d(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - is_planar
 * If vtable method is NULL, assumes planar if 2D
 */
bool sylves_grid_default_is_planar(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - is_repeating
 * If vtable method is NULL, checks grid type for common patterns
 */
bool sylves_grid_default_is_repeating(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - is_orientable  
 * If vtable method is NULL, assumes true for most grids
 */
bool sylves_grid_default_is_orientable(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - is_finite
 * If vtable method is NULL, returns true if bound is set
 */
bool sylves_grid_default_is_finite(const SylvesGrid* grid);

/**
 * @brief Get property with fallback - coordinate dimension
 * If vtable method is NULL, checks cell types or assumes based on is_2d/is_3d
 */
int sylves_grid_default_coordinate_dimension(const SylvesGrid* grid);


#endif /* GRID_DEFAULTS_H */
