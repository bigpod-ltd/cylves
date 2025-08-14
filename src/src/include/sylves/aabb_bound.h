/**
 * @file aabb_bound.h
 * @brief AabbBound type for axis-aligned bounding box constraints
 */

#ifndef SYLVES_AABB_BOUND_H
#define SYLVES_AABB_BOUND_H

#include "types.h"
#include "bounds.h"

/* AabbBound - Continuous axis-aligned bounding box constraints */

/**
 * Create a 2D AabbBound
 * 
 * @param min_x Minimum x coordinate
 * @param min_y Minimum y coordinate
 * @param max_x Maximum x coordinate
 * @param max_y Maximum y coordinate
 * @return New AABB bound or NULL on error
 */
SylvesBound* sylves_bound_create_aabb_2d(float min_x, float min_y, float max_x, float max_y);

/**
 * Create a 3D AabbBound
 * 
 * @param min_x Minimum x coordinate
 * @param min_y Minimum y coordinate
 * @param min_z Minimum z coordinate
 * @param max_x Maximum x coordinate
 * @param max_y Maximum y coordinate
 * @param max_z Maximum z coordinate
 * @return New AABB bound or NULL on error
 */
SylvesBound* sylves_bound_create_aabb_3d(float min_x, float min_y, float min_z,
                                         float max_x, float max_y, float max_z);

/**
 * Create an AabbBound from a base grid and cell bound
 * 
 * @param grid Grid to query for cell AABBs
 * @param cell_bound Bound defining which cells to include
 * @return New AABB bound or NULL on error
 */
SylvesBound* sylves_bound_create_aabb_from_cells(const SylvesGrid* grid, const SylvesBound* cell_bound);

/**
 * Get the continuous bounds of an AabbBound
 * 
 * @param bound AabbBound to query
 * @param min Output minimum corner (must have space for 3 floats)
 * @param max Output maximum corner (must have space for 3 floats)
 * @return 0 on success, negative error code on failure
 */
int sylves_aabb_bound_get_bounds(const SylvesBound* bound, float* min, float* max);

/**
 * Check if a point is within the AabbBound
 * 
 * @param bound AabbBound to query
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z coordinate
 * @return true if point is within bounds, false otherwise
 */
bool sylves_aabb_bound_contains_point(const SylvesBound* bound, float x, float y, float z);

/**
 * Expand an AabbBound by a margin
 * 
 * @param bound AabbBound to expand
 * @param margin Amount to expand in all directions
 * @return New expanded bound or NULL on error
 */
SylvesBound* sylves_aabb_bound_expand(const SylvesBound* bound, float margin);

/**
 * Get the dimensions of an AabbBound
 * 
 * @param bound AabbBound to query
 * @return 2 for 2D bounds, 3 for 3D bounds, or negative error code
 */
int sylves_aabb_bound_get_dimensions(const SylvesBound* bound);

#endif /* SYLVES_AABB_BOUND_H */
