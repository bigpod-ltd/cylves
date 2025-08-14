/**
 * @file aabb.c
 * @brief Axis-aligned bounding box operations implementation
 */

#include "sylves/aabb.h"
#include "sylves/vector.h"
#include <float.h>

SylvesAabb sylves_aabb_create(SylvesVector3 min, SylvesVector3 max) {
    SylvesAabb aabb = {min, max};
    return aabb;
}

SylvesAabb sylves_aabb_create_empty(void) {
    SylvesAabb aabb = {
        sylves_vector3_create(DBL_MAX, DBL_MAX, DBL_MAX),
        sylves_vector3_create(-DBL_MAX, -DBL_MAX, -DBL_MAX)
    };
    return aabb;
}

SylvesAabb sylves_aabb_create_from_center_size(SylvesVector3 center, SylvesVector3 size) {
    SylvesVector3 half_size = sylves_vector3_scale(size, 0.5);
    return sylves_aabb_create(
        sylves_vector3_subtract(center, half_size),
        sylves_vector3_add(center, half_size)
    );
}

SylvesAabb sylves_aabb_create_from_points(const SylvesVector3* points, size_t count) {
    if (count == 0) {
        return sylves_aabb_create_empty();
    }
    
    SylvesVector3 min = points[0];
    SylvesVector3 max = points[0];
    
    for (size_t i = 1; i < count; i++) {
        min = sylves_vector3_min(min, points[i]);
        max = sylves_vector3_max(max, points[i]);
    }
    
    return sylves_aabb_create(min, max);
}

bool sylves_aabb_is_valid(SylvesAabb aabb) {
    return aabb.min.x <= aabb.max.x && 
           aabb.min.y <= aabb.max.y && 
           aabb.min.z <= aabb.max.z;
}

bool sylves_aabb_is_empty(SylvesAabb aabb) {
    return aabb.min.x > aabb.max.x || 
           aabb.min.y > aabb.max.y || 
           aabb.min.z > aabb.max.z;
}

SylvesVector3 sylves_aabb_get_center(SylvesAabb aabb) {
    return sylves_vector3_scale(sylves_vector3_add(aabb.min, aabb.max), 0.5);
}

SylvesVector3 sylves_aabb_get_size(SylvesAabb aabb) {
    if (sylves_aabb_is_empty(aabb)) {
        return sylves_vector3_zero();
    }
    return sylves_vector3_subtract(aabb.max, aabb.min);
}

SylvesVector3 sylves_aabb_get_extents(SylvesAabb aabb) {
    return sylves_vector3_scale(sylves_aabb_get_size(aabb), 0.5);
}

double sylves_aabb_get_volume(SylvesAabb aabb) {
    if (sylves_aabb_is_empty(aabb)) {
        return 0.0;
    }
    SylvesVector3 size = sylves_aabb_get_size(aabb);
    return size.x * size.y * size.z;
}

double sylves_aabb_get_surface_area(SylvesAabb aabb) {
    if (sylves_aabb_is_empty(aabb)) {
        return 0.0;
    }
    SylvesVector3 size = sylves_aabb_get_size(aabb);
    return 2.0 * (size.x * size.y + size.x * size.z + size.y * size.z);
}

bool sylves_aabb_contains_point(SylvesAabb aabb, SylvesVector3 point) {
    return point.x >= aabb.min.x && point.x <= aabb.max.x &&
           point.y >= aabb.min.y && point.y <= aabb.max.y &&
           point.z >= aabb.min.z && point.z <= aabb.max.z;
}

bool sylves_aabb_contains_aabb(SylvesAabb a, SylvesAabb b) {
    return a.min.x <= b.min.x && a.max.x >= b.max.x &&
           a.min.y <= b.min.y && a.max.y >= b.max.y &&
           a.min.z <= b.min.z && a.max.z >= b.max.z;
}

bool sylves_aabb_intersects(SylvesAabb a, SylvesAabb b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

SylvesAabb sylves_aabb_merge(SylvesAabb a, SylvesAabb b) {
    if (sylves_aabb_is_empty(a)) return b;
    if (sylves_aabb_is_empty(b)) return a;
    
    return sylves_aabb_create(
        sylves_vector3_min(a.min, b.min),
        sylves_vector3_max(a.max, b.max)
    );
}

SylvesAabb sylves_aabb_intersect(SylvesAabb a, SylvesAabb b) {
    return sylves_aabb_create(
        sylves_vector3_max(a.min, b.min),
        sylves_vector3_min(a.max, b.max)
    );
}

SylvesAabb sylves_aabb_expand(SylvesAabb aabb, double amount) {
    SylvesVector3 expansion = sylves_vector3_create(amount, amount, amount);
    return sylves_aabb_create(
        sylves_vector3_subtract(aabb.min, expansion),
        sylves_vector3_add(aabb.max, expansion)
    );
}

SylvesAabb sylves_aabb_expand_to_include(SylvesAabb aabb, SylvesVector3 point) {
    return sylves_aabb_create(
        sylves_vector3_min(aabb.min, point),
        sylves_vector3_max(aabb.max, point)
    );
}

SylvesAabb sylves_aabb_transform(SylvesAabb aabb, const SylvesMatrix4x4* matrix) {
    // Transform all 8 corners of the AABB
    SylvesVector3 corners[8] = {
        sylves_vector3_create(aabb.min.x, aabb.min.y, aabb.min.z),
        sylves_vector3_create(aabb.max.x, aabb.min.y, aabb.min.z),
        sylves_vector3_create(aabb.min.x, aabb.max.y, aabb.min.z),
        sylves_vector3_create(aabb.max.x, aabb.max.y, aabb.min.z),
        sylves_vector3_create(aabb.min.x, aabb.min.y, aabb.max.z),
        sylves_vector3_create(aabb.max.x, aabb.min.y, aabb.max.z),
        sylves_vector3_create(aabb.min.x, aabb.max.y, aabb.max.z),
        sylves_vector3_create(aabb.max.x, aabb.max.y, aabb.max.z)
    };
    
    // Transform each corner
    for (int i = 0; i < 8; i++) {
        corners[i] = sylves_matrix4x4_multiply_point(matrix, corners[i]);
    }
    
    // Create new AABB from transformed corners
    return sylves_aabb_create_from_points(corners, 8);
}

SylvesVector3 sylves_aabb_closest_point(SylvesAabb aabb, SylvesVector3 point) {
    return sylves_vector3_create(
        fmax(aabb.min.x, fmin(point.x, aabb.max.x)),
        fmax(aabb.min.y, fmin(point.y, aabb.max.y)),
        fmax(aabb.min.z, fmin(point.z, aabb.max.z))
    );
}

double sylves_aabb_distance_to_point(SylvesAabb aabb, SylvesVector3 point) {
    SylvesVector3 closest = sylves_aabb_closest_point(aabb, point);
    return sylves_vector3_distance(point, closest);
}

bool sylves_aabb_ray_intersect(SylvesAabb aabb, SylvesVector3 origin, SylvesVector3 direction, double* t_min, double* t_max) {
    double tmin = 0.0;
    double tmax = DBL_MAX;
    
    // Check each axis
    for (int i = 0; i < 3; i++) {
        double origin_i = (i == 0) ? origin.x : (i == 1) ? origin.y : origin.z;
        double dir_i = (i == 0) ? direction.x : (i == 1) ? direction.y : direction.z;
        double min_i = (i == 0) ? aabb.min.x : (i == 1) ? aabb.min.y : aabb.min.z;
        double max_i = (i == 0) ? aabb.max.x : (i == 1) ? aabb.max.y : aabb.max.z;
        
        if (fabs(dir_i) < 1e-6) {
            // Ray is parallel to slab
            if (origin_i < min_i || origin_i > max_i) {
                return false;
            }
        } else {
            // Compute intersection distances
            double t1 = (min_i - origin_i) / dir_i;
            double t2 = (max_i - origin_i) / dir_i;
            
            if (t1 > t2) {
                // Swap
                double temp = t1;
                t1 = t2;
                t2 = temp;
            }
            
            tmin = fmax(tmin, t1);
            tmax = fmin(tmax, t2);
            
            if (tmin > tmax) {
                return false;
            }
        }
    }
    
    if (t_min) *t_min = tmin;
    if (t_max) *t_max = tmax;
    
    return true;
}
