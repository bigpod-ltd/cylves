/**
 * @file geometry_utils.c
 * @brief Computational geometry utilities implementation
 */

#include "sylves/geometry_utils.h"
#include "sylves/vector.h"
#include "sylves/delaunay.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* Helper: Cross product of 2D vectors (returns z component) */
static float cross_2d(const SylvesVector2* a, const SylvesVector2* b) {
    return a->x * b->y - a->y * b->x;
}

/* Helper: Winding number for point in polygon */
static int winding_number_2d(const SylvesVector2* point, const SylvesVector2* polygon, size_t n) {
    int winding = 0;
    
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        
        if (polygon[i].y <= point->y) {
            if (polygon[j].y > point->y) {
                /* Upward crossing */
                if (sylves_orient2d(polygon[i].x, polygon[i].y,
                                   polygon[j].x, polygon[j].y,
                                   point->x, point->y)) {
                    winding++;
                }
            }
        } else {
            if (polygon[j].y <= point->y) {
                /* Downward crossing */
                if (!sylves_orient2d(polygon[i].x, polygon[i].y,
                                    polygon[j].x, polygon[j].y,
                                    point->x, point->y)) {
                    winding--;
                }
            }
        }
    }
    
    return winding;
}

bool sylves_point_in_polygon_2d(
    const SylvesVector2* point,
    const SylvesVector2* polygon,
    size_t num_vertices
) {
    if (!point || !polygon || num_vertices < 3) {
        return false;
    }
    
    return winding_number_2d(point, polygon, num_vertices) != 0;
}

bool sylves_point_in_polygon_3d(
    const SylvesVector3* point,
    const SylvesVector3* polygon,
    size_t num_vertices,
    const SylvesVector3* normal
) {
    if (!point || !polygon || num_vertices < 3) {
        return false;
    }
    
    /* Compute normal if not provided */
    SylvesVector3 n;
    if (normal) {
        n = *normal;
    } else {
        /* Newell's method for polygon normal */
        n = (SylvesVector3){0, 0, 0};
        for (size_t i = 0; i < num_vertices; i++) {
            size_t j = (i + 1) % num_vertices;
            n.x += (polygon[i].y - polygon[j].y) * (polygon[i].z + polygon[j].z);
            n.y += (polygon[i].z - polygon[j].z) * (polygon[i].x + polygon[j].x);
            n.z += (polygon[i].x - polygon[j].x) * (polygon[i].y + polygon[j].y);
        }
        n = sylves_vector3_normalize(n);
    }
    
    /* Find best projection plane */
    int axis = 0;
    float max_comp = fabsf(n.x);
    if (fabsf(n.y) > max_comp) { axis = 1; max_comp = fabsf(n.y); }
    if (fabsf(n.z) > max_comp) { axis = 2; }
    
    /* Project to 2D */
    SylvesVector2* poly2d = malloc(num_vertices * sizeof(SylvesVector2));
    if (!poly2d) return false;
    
    SylvesVector2 pt2d;
    switch (axis) {
        case 0: /* Project to YZ plane */
            pt2d = (SylvesVector2){point->y, point->z};
            for (size_t i = 0; i < num_vertices; i++) {
                poly2d[i] = (SylvesVector2){polygon[i].y, polygon[i].z};
            }
            break;
        case 1: /* Project to XZ plane */
            pt2d = (SylvesVector2){point->x, point->z};
            for (size_t i = 0; i < num_vertices; i++) {
                poly2d[i] = (SylvesVector2){polygon[i].x, polygon[i].z};
            }
            break;
        case 2: /* Project to XY plane */
            pt2d = (SylvesVector2){point->x, point->y};
            for (size_t i = 0; i < num_vertices; i++) {
                poly2d[i] = (SylvesVector2){polygon[i].x, polygon[i].y};
            }
            break;
    }
    
    bool result = sylves_point_in_polygon_2d(&pt2d, poly2d, num_vertices);
    free(poly2d);
    
    return result;
}

bool sylves_line_segment_intersection_2d(
    const SylvesVector2* a0,
    const SylvesVector2* a1,
    const SylvesVector2* b0,
    const SylvesVector2* b1,
    SylvesVector2* intersection_out,
    float* t_out,
    float* u_out
) {
    SylvesVector2 da = {a1->x - a0->x, a1->y - a0->y};
    SylvesVector2 db = {b1->x - b0->x, b1->y - b0->y};
    SylvesVector2 dc = {b0->x - a0->x, b0->y - a0->y};
    
    float cross_da_db = cross_2d(&da, &db);
    
    /* Check if lines are parallel */
    if (fabsf(cross_da_db) < 1e-6f) {
        return false;
    }
    
    float t = cross_2d(&dc, &db) / cross_da_db;
    float u = cross_2d(&dc, &da) / cross_da_db;
    
    /* Check if intersection is within both segments */
    if (t < 0 || t > 1 || u < 0 || u > 1) {
        return false;
    }
    
    if (intersection_out) {
        intersection_out->x = a0->x + t * da.x;
        intersection_out->y = a0->y + t * da.y;
    }
    
    if (t_out) *t_out = t;
    if (u_out) *u_out = u;
    
    return true;
}

bool sylves_line_intersection_2d(
    const SylvesVector2* p0,
    const SylvesVector2* d0,
    const SylvesVector2* p1,
    const SylvesVector2* d1,
    SylvesVector2* intersection_out
) {
    float cross_d0_d1 = cross_2d(d0, d1);
    
    /* Check if lines are parallel */
    if (fabsf(cross_d0_d1) < 1e-6f) {
        return false;
    }
    
    SylvesVector2 dp = {p1->x - p0->x, p1->y - p0->y};
    float t = cross_2d(&dp, d1) / cross_d0_d1;
    
    if (intersection_out) {
        intersection_out->x = p0->x + t * d0->x;
        intersection_out->y = p0->y + t * d0->y;
    }
    
    return true;
}

/* Helper for convex hull: Compare angles for sorting */
static int compare_polar_angle(const void* a, const void* b, void* context) {
    const int* ia = (const int*)a;
    const int* ib = (const int*)b;
    const SylvesVector2* points = (const SylvesVector2*)context;
    const SylvesVector2* origin = &points[0];
    
    SylvesVector2 va = {points[*ia].x - origin->x, points[*ia].y - origin->y};
    SylvesVector2 vb = {points[*ib].x - origin->x, points[*ib].y - origin->y};
    
    float cross = cross_2d(&va, &vb);
    if (cross > 0) return -1;
    if (cross < 0) return 1;
    
    /* Collinear - sort by distance */
    float da = va.x * va.x + va.y * va.y;
    float db = vb.x * vb.x + vb.y * vb.y;
    return (da < db) ? -1 : (da > db) ? 1 : 0;
}

bool sylves_convex_hull_2d(
    const SylvesVector2* points,
    size_t num_points,
    int* hull_out,
    size_t* hull_size_out
) {
    if (!points || !hull_out || !hull_size_out || num_points < 3) {
        return false;
    }
    
    /* Find bottommost point (and leftmost if tied) */
    int bottom = 0;
    for (size_t i = 1; i < num_points; i++) {
        if (points[i].y < points[bottom].y ||
            (points[i].y == points[bottom].y && points[i].x < points[bottom].x)) {
            bottom = (int)i;
        }
    }
    
    /* Create index array for sorting */
    int* indices = malloc(num_points * sizeof(int));
    if (!indices) return false;
    
    for (size_t i = 0; i < num_points; i++) {
        indices[i] = (int)i;
    }
    
    /* Swap bottom point to index 0 */
    indices[bottom] = indices[0];
    indices[0] = bottom;
    
    /* Sort by polar angle with respect to bottom point */
    /* Note: Using simple O(n²) sort since qsort_r isn't portable */
    SylvesVector2 origin = points[bottom];
    for (size_t i = 2; i < num_points; i++) {
        for (size_t j = i; j > 1; j--) {
            SylvesVector2 v1 = {
                points[indices[j]].x - origin.x,
                points[indices[j]].y - origin.y
            };
            SylvesVector2 v2 = {
                points[indices[j-1]].x - origin.x,
                points[indices[j-1]].y - origin.y
            };
            
            float cross = cross_2d(&v2, &v1);
            if (cross < 0 || (cross == 0 && 
                v1.x*v1.x + v1.y*v1.y < v2.x*v2.x + v2.y*v2.y)) {
                int tmp = indices[j];
                indices[j] = indices[j-1];
                indices[j-1] = tmp;
            } else {
                break;
            }
        }
    }
    
    /* Graham scan */
    size_t hull_size = 0;
    
    /* Add first two points */
    hull_out[hull_size++] = indices[0];
    hull_out[hull_size++] = indices[1];
    
    for (size_t i = 2; i < num_points; i++) {
        /* Remove points that make clockwise turn */
        while (hull_size > 1) {
            SylvesVector2 v1 = {
                points[hull_out[hull_size-1]].x - points[hull_out[hull_size-2]].x,
                points[hull_out[hull_size-1]].y - points[hull_out[hull_size-2]].y
            };
            SylvesVector2 v2 = {
                points[indices[i]].x - points[hull_out[hull_size-1]].x,
                points[indices[i]].y - points[hull_out[hull_size-1]].y
            };
            
            if (cross_2d(&v1, &v2) <= 0) {
                hull_size--;
            } else {
                break;
            }
        }
        
        hull_out[hull_size++] = indices[i];
    }
    
    free(indices);
    *hull_size_out = hull_size;
    
    return true;
}

bool sylves_convex_hull_3d(
    const SylvesVector3* points,
    size_t num_points,
    int* faces_out,
    size_t max_faces,
    size_t* num_faces_out
) {
    /* Simple implementation using incremental algorithm */
    /* For production, consider using QuickHull or similar */
    
    if (!points || !faces_out || !num_faces_out || num_points < 4) {
        return false;
    }
    
    /* Find initial tetrahedron */
    /* TODO: Implement full 3D convex hull */
    /* For now, return false to indicate not implemented */
    
    return false;
}

float sylves_point_to_segment_distance_2d(
    const SylvesVector2* point,
    const SylvesVector2* seg_start,
    const SylvesVector2* seg_end,
    SylvesVector2* closest_out
) {
    SylvesVector2 seg = {seg_end->x - seg_start->x, seg_end->y - seg_start->y};
    SylvesVector2 to_point = {point->x - seg_start->x, point->y - seg_start->y};
    
    float seg_length_sq = seg.x * seg.x + seg.y * seg.y;
    if (seg_length_sq < 1e-6f) {
        /* Degenerate segment */
        if (closest_out) *closest_out = *seg_start;
        return sqrtf(to_point.x * to_point.x + to_point.y * to_point.y);
    }
    
    float t = (to_point.x * seg.x + to_point.y * seg.y) / seg_length_sq;
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    SylvesVector2 closest = {
        seg_start->x + t * seg.x,
        seg_start->y + t * seg.y
    };
    
    if (closest_out) *closest_out = closest;
    
    float dx = point->x - closest.x;
    float dy = point->y - closest.y;
    return sqrtf(dx * dx + dy * dy);
}

float sylves_point_to_line_distance_2d(
    const SylvesVector2* point,
    const SylvesVector2* line_point,
    const SylvesVector2* line_dir
) {
    /* Distance = |((P - L) × D)| / |D| */
    SylvesVector2 to_point = {
        point->x - line_point->x,
        point->y - line_point->y
    };
    
    float cross = cross_2d(&to_point, line_dir);
    float dir_length = sqrtf(line_dir->x * line_dir->x + line_dir->y * line_dir->y);
    
    return fabsf(cross) / dir_length;
}

float sylves_polygon_area_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
) {
    if (!vertices || num_vertices < 3) return 0.0f;
    
    /* Shoelace formula */
    float area = 0.0f;
    for (size_t i = 0; i < num_vertices; i++) {
        size_t j = (i + 1) % num_vertices;
        area += vertices[i].x * vertices[j].y;
        area -= vertices[j].x * vertices[i].y;
    }
    
    return area * 0.5f;
}

float sylves_polygon_perimeter_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
) {
    if (!vertices || num_vertices < 2) return 0.0f;
    
    float perimeter = 0.0f;
    for (size_t i = 0; i < num_vertices; i++) {
        size_t j = (i + 1) % num_vertices;
        float dx = vertices[j].x - vertices[i].x;
        float dy = vertices[j].y - vertices[i].y;
        perimeter += sqrtf(dx * dx + dy * dy);
    }
    
    return perimeter;
}

SylvesVector2 sylves_polygon_centroid_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
) {
    if (!vertices || num_vertices == 0) {
        return (SylvesVector2){0, 0};
    }
    
    float area = 0.0f;
    float cx = 0.0f;
    float cy = 0.0f;
    
    for (size_t i = 0; i < num_vertices; i++) {
        size_t j = (i + 1) % num_vertices;
        float a = vertices[i].x * vertices[j].y - vertices[j].x * vertices[i].y;
        area += a;
        cx += (vertices[i].x + vertices[j].x) * a;
        cy += (vertices[i].y + vertices[j].y) * a;
    }
    
    if (fabsf(area) < 1e-6f) {
        /* Degenerate polygon - return average of vertices */
        cx = cy = 0.0f;
        for (size_t i = 0; i < num_vertices; i++) {
            cx += vertices[i].x;
            cy += vertices[i].y;
        }
        return (SylvesVector2){cx / num_vertices, cy / num_vertices};
    }
    
    area *= 3.0f;
    return (SylvesVector2){cx / area, cy / area};
}

void sylves_compute_bbox_2d(
    const SylvesVector2* points,
    size_t num_points,
    SylvesVector2* min_out,
    SylvesVector2* max_out
) {
    if (!points || num_points == 0 || !min_out || !max_out) return;
    
    *min_out = *max_out = points[0];
    
    for (size_t i = 1; i < num_points; i++) {
        min_out->x = fminf(min_out->x, points[i].x);
        min_out->y = fminf(min_out->y, points[i].y);
        max_out->x = fmaxf(max_out->x, points[i].x);
        max_out->y = fmaxf(max_out->y, points[i].y);
    }
}

void sylves_compute_bbox_3d(
    const SylvesVector3* points,
    size_t num_points,
    SylvesVector3* min_out,
    SylvesVector3* max_out
) {
    if (!points || num_points == 0 || !min_out || !max_out) return;
    
    *min_out = *max_out = points[0];
    
    for (size_t i = 1; i < num_points; i++) {
        min_out->x = fminf(min_out->x, points[i].x);
        min_out->y = fminf(min_out->y, points[i].y);
        min_out->z = fminf(min_out->z, points[i].z);
        max_out->x = fmaxf(max_out->x, points[i].x);
        max_out->y = fmaxf(max_out->y, points[i].y);
        max_out->z = fmaxf(max_out->z, points[i].z);
    }
}

/* Helper: Check if triangle contains any other vertices */
static bool is_ear(const SylvesVector2* vertices, size_t num_vertices,
                   size_t prev, size_t curr, size_t next) {
    /* Check if triangle is CCW */
    if (!sylves_orient2d(vertices[prev].x, vertices[prev].y,
                        vertices[curr].x, vertices[curr].y,
                        vertices[next].x, vertices[next].y)) {
        return false;
    }
    
    /* Check if any other vertex is inside triangle */
    for (size_t i = 0; i < num_vertices; i++) {
        if (i == prev || i == curr || i == next) continue;
        
        /* Check if point is inside triangle using barycentric coordinates */
        SylvesVector2 v0 = {vertices[next].x - vertices[prev].x,
                           vertices[next].y - vertices[prev].y};
        SylvesVector2 v1 = {vertices[curr].x - vertices[prev].x,
                           vertices[curr].y - vertices[prev].y};
        SylvesVector2 v2 = {vertices[i].x - vertices[prev].x,
                           vertices[i].y - vertices[prev].y};
        
        float d00 = v0.x * v0.x + v0.y * v0.y;
        float d01 = v0.x * v1.x + v0.y * v1.y;
        float d11 = v1.x * v1.x + v1.y * v1.y;
        float d20 = v2.x * v0.x + v2.y * v0.y;
        float d21 = v2.x * v1.x + v2.y * v1.y;
        
        float denom = d00 * d11 - d01 * d01;
        if (fabsf(denom) < 1e-6f) continue;
        
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        
        if (u >= 0 && v >= 0 && w >= 0) {
            return false; /* Point is inside */
        }
    }
    
    return true;
}

bool sylves_triangulate_polygon_2d(
    const SylvesVector2* vertices,
    size_t num_vertices,
    int* triangles_out,
    size_t max_triangles,
    size_t* num_triangles_out
) {
    if (!vertices || !triangles_out || !num_triangles_out || 
        num_vertices < 3 || max_triangles < num_vertices - 2) {
        return false;
    }
    
    /* Create vertex index list */
    int* indices = malloc(num_vertices * sizeof(int));
    if (!indices) return false;
    
    for (size_t i = 0; i < num_vertices; i++) {
        indices[i] = (int)i;
    }
    
    size_t num_triangles = 0;
    size_t remaining = num_vertices;
    
    /* Ear clipping algorithm */
    while (remaining > 3) {
        bool found_ear = false;
        
        for (size_t i = 0; i < remaining; i++) {
            size_t prev = (i + remaining - 1) % remaining;
            size_t next = (i + 1) % remaining;
            
            if (is_ear(vertices, num_vertices, 
                      indices[prev], indices[i], indices[next])) {
                /* Add triangle */
                triangles_out[num_triangles * 3] = indices[prev];
                triangles_out[num_triangles * 3 + 1] = indices[i];
                triangles_out[num_triangles * 3 + 2] = indices[next];
                num_triangles++;
                
                /* Remove vertex from list */
                for (size_t j = i; j < remaining - 1; j++) {
                    indices[j] = indices[j + 1];
                }
                remaining--;
                
                found_ear = true;
                break;
            }
        }
        
        if (!found_ear) {
            /* Polygon is not simple or has collinear vertices */
            free(indices);
            return false;
        }
    }
    
    /* Add final triangle */
    triangles_out[num_triangles * 3] = indices[0];
    triangles_out[num_triangles * 3 + 1] = indices[1];
    triangles_out[num_triangles * 3 + 2] = indices[2];
    num_triangles++;
    
    free(indices);
    *num_triangles_out = num_triangles;
    
    return true;
}
