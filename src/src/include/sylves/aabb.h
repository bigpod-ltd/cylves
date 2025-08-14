/**
 * @file aabb.h
 * @brief Axis-aligned bounding box operations
 */

#ifndef SYLVES_AABB_H
#define SYLVES_AABB_H

#include "types.h"
#include "vector.h"
#include "matrix.h"
#include <stdbool.h>
#include <stddef.h>


/* AABB creation */
SylvesAabb sylves_aabb_create(SylvesVector3 min, SylvesVector3 max);
SylvesAabb sylves_aabb_create_empty(void);
SylvesAabb sylves_aabb_create_from_center_size(SylvesVector3 center, SylvesVector3 size);
SylvesAabb sylves_aabb_create_from_points(const SylvesVector3* points, size_t count);

/* AABB queries */
bool sylves_aabb_is_valid(SylvesAabb aabb);
bool sylves_aabb_is_empty(SylvesAabb aabb);
SylvesVector3 sylves_aabb_get_center(SylvesAabb aabb);
SylvesVector3 sylves_aabb_get_size(SylvesAabb aabb);
SylvesVector3 sylves_aabb_get_extents(SylvesAabb aabb);
double sylves_aabb_get_volume(SylvesAabb aabb);
double sylves_aabb_get_surface_area(SylvesAabb aabb);

/* AABB containment and intersection */
bool sylves_aabb_contains_point(SylvesAabb aabb, SylvesVector3 point);
bool sylves_aabb_contains_aabb(SylvesAabb a, SylvesAabb b);
bool sylves_aabb_intersects(SylvesAabb a, SylvesAabb b);

/* AABB operations */
SylvesAabb sylves_aabb_merge(SylvesAabb a, SylvesAabb b);
SylvesAabb sylves_aabb_intersect(SylvesAabb a, SylvesAabb b);
SylvesAabb sylves_aabb_expand(SylvesAabb aabb, double amount);
SylvesAabb sylves_aabb_expand_to_include(SylvesAabb aabb, SylvesVector3 point);
SylvesAabb sylves_aabb_transform(SylvesAabb aabb, const SylvesMatrix4x4* matrix);

/* AABB distance operations */
SylvesVector3 sylves_aabb_closest_point(SylvesAabb aabb, SylvesVector3 point);
double sylves_aabb_distance_to_point(SylvesAabb aabb, SylvesVector3 point);

/* AABB ray intersection */
bool sylves_aabb_ray_intersect(SylvesAabb aabb, SylvesVector3 origin, SylvesVector3 direction, double* t_min, double* t_max);


#endif /* SYLVES_AABB_H */
