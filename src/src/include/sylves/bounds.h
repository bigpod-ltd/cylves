/**
 * @file bounds.h
 * @brief Bounds and bounding operations
 */

#ifndef SYLVES_BOUNDS_H
#define SYLVES_BOUNDS_H

#include "types.h"
#include <stddef.h>


/* Bound types */
typedef enum {
    SYLVES_BOUND_TYPE_UNKNOWN = 0,
    SYLVES_BOUND_TYPE_RECT = 1,
    SYLVES_BOUND_TYPE_CUBE = 2,
    SYLVES_BOUND_TYPE_HEX  = 3,
    SYLVES_BOUND_TYPE_TRIANGLE = 4,
    SYLVES_BOUND_TYPE_MASK = 5,
    SYLVES_BOUND_TYPE_AABB = 6,
    SYLVES_BOUND_CUBE = 2,  /* Alias for compatibility */
} SylvesBoundType;

/* Query bound type */
SylvesBoundType sylves_bound_get_type(const SylvesBound* bound);

/* Bounds operations - rectangle, cube, and hex (basic) */
SylvesBound* sylves_bound_create_rectangle(int min_x, int min_y, int max_x, int max_y);
SylvesBound* sylves_bound_create_cube(int min_x, int min_y, int min_z,
                                      int max_x, int max_y, int max_z);
/* Hex axial parallelogram bound (q,r ranges) */
SylvesBound* sylves_bound_create_hex_parallelogram(int min_q, int min_r, int max_q, int max_r);
void sylves_bound_destroy(SylvesBound* bound);
bool sylves_bound_contains(const SylvesBound* bound, SylvesCell cell);

/* Hex-specific getters (Min/Mex, exclusive upper bound) when bound is Hex */
int sylves_hex_bound_get_min_mex(const SylvesBound* bound,
                                 int* min_x, int* min_y, int* min_z,
                                 int* mex_x, int* mex_y, int* mex_z);

/* Triangle parallelogram bound (x,y,z ranges where x+y+z=1 or 2) */
SylvesBound* sylves_bound_create_triangle_parallelogram(int min_x, int min_y, int min_z,
                                                        int max_x, int max_y, int max_z);

/* Rectangle-specific helpers */
SylvesBound* sylves_bound_intersect(const SylvesBound* a, const SylvesBound* b);
SylvesBound* sylves_bound_union(const SylvesBound* a, const SylvesBound* b);

/* Enumerate cells in a bound. Rectangle: 2D; Cube: 3D. Returns count written or error. */
int sylves_bound_get_cells(const SylvesBound* bound, SylvesCell* cells, size_t max_cells);

/* Get extents. Returns 0 on success, negative error otherwise */
int sylves_bound_get_rect(const SylvesBound* bound, int* min_x, int* min_y, int* max_x, int* max_y);
int sylves_bound_get_cube(const SylvesBound* bound, int* min_x, int* min_y, int* min_z,
                          int* max_x, int* max_y, int* max_z);

/* CubeBound specific functions */
SylvesBound* sylves_cube_bound_create(int min_x, int min_y, int min_z,
                                      int max_x, int max_y, int max_z);
int sylves_cube_bound_get_min_x(const SylvesBound* bound);
int sylves_cube_bound_get_min_y(const SylvesBound* bound);
int sylves_cube_bound_get_min_z(const SylvesBound* bound);
int sylves_cube_bound_get_max_x(const SylvesBound* bound);
int sylves_cube_bound_get_max_y(const SylvesBound* bound);
int sylves_cube_bound_get_max_z(const SylvesBound* bound);

/* Additional bound operations */
int sylves_bound_get_cell_count(const SylvesBound* bound);
SylvesBound* sylves_bound_clone(const SylvesBound* bound);
bool sylves_bound_is_empty(const SylvesBound* bound);
int sylves_bound_get_aabb(const SylvesBound* bound, float* min, float* max);

/* Enhanced intersection/union with vtable dispatch */
SylvesBound* sylves_bound_intersect_ex(const SylvesBound* a, const SylvesBound* b);
SylvesBound* sylves_bound_union_ex(const SylvesBound* a, const SylvesBound* b);

/* Include bound type headers */
#include "mask_bound.h"
#include "aabb_bound.h"

#endif /* SYLVES_BOUNDS_H */
