/**
 * @file geometry_utils.h
 * @brief Computational geometry utilities
 */

#ifndef SYLVES_GEOMETRY_UTILS_H
#define SYLVES_GEOMETRY_UTILS_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Point-in-polygon tests */

/**
 * @brief Test if point is inside 2D polygon
 * 
 * Uses winding number algorithm for robust testing.
 * 
 * @param point Point to test
 * @param polygon Array of polygon vertices (must be closed)
 * @param num_vertices Number of vertices in polygon
 * @return true if point is inside polygon
 */
bool sylves_point_in_polygon_2d(
    const SylvesVector2* point,
    const SylvesVector2* polygon,
    size_t num_vertices
);

/**
 * @brief Test if point is inside 3D polygon
 * 
 * Projects to best-fit plane and performs 2D test.
 * 
 * @param point Point to test
 * @param polygon Array of polygon vertices
 * @param num_vertices Number of vertices
 * @param normal Optional normal vector (computed if NULL)
 * @return true if point is inside polygon
 */
bool sylves_point_in_polygon_3d(
    const SylvesVector3* point,
    const SylvesVector3* polygon,
    size_t num_vertices,
    const SylvesVector3* normal
);

/* Line intersection */

/**
 * @brief Compute intersection of two 2D line segments
 * 
 * @param a0 Start of first segment
 * @param a1 End of first segment
 * @param b0 Start of second segment
 * @param b1 End of second segment
 * @param intersection_out Output intersection point
 * @param t_out Parameter along first segment (0-1)
 * @param u_out Parameter along second segment (0-1)
 * @return true if segments intersect
 */
bool sylves_line_segment_intersection_2d(
    const SylvesVector2* a0,
    const SylvesVector2* a1,
    const SylvesVector2* b0,
    const SylvesVector2* b1,
    SylvesVector2* intersection_out,
    float* t_out,
    float* u_out
);

/**
 * @brief Compute intersection of two infinite 2D lines
 * 
 * @param p0 Point on first line
 * @param d0 Direction of first line
 * @param p1 Point on second line
 * @param d1 Direction of second line
 * @param intersection_out Output intersection point
 * @return true if lines intersect (not parallel)
 */
bool sylves_line_intersection_2d(
    const SylvesVector2* p0,
    const SylvesVector2* d0,
    const SylvesVector2* p1,
    const SylvesVector2* d1,
    SylvesVector2* intersection_out
);

/* Convex hull */

/**
 * @brief Compute 2D convex hull using Graham scan
 * 
 * @param points Input points
 * @param num_points Number of input points
 * @param hull_out Array to store hull indices (must be large enough)
 * @param hull_size_out Number of hull vertices
 * @return true on success
 */
bool sylves_convex_hull_2d(
    const SylvesVector2* points,
    size_t num_points,
    int* hull_out,
    size_t* hull_size_out
);

/**
 * @brief Compute 3D convex hull
 * 
 * Uses incremental algorithm.
 * 
 * @param points Input points
 * @param num_points Number of points
 * @param faces_out Array to store face indices (3 per face)
 * @param max_faces Maximum faces that can be stored
 * @param num_faces_out Number of faces in hull
 * @return true on success
 */
bool sylves_convex_hull_3d(
    const SylvesVector3* points,
    size_t num_points,
    int* faces_out,
    size_t max_faces,
    size_t* num_faces_out
);

/* Distance queries */

/**
 * @brief Distance from point to line segment
 * 
 * @param point Query point
 * @param seg_start Segment start
 * @param seg_end Segment end
 * @param closest_out Optional closest point on segment
 * @return Distance to segment
 */
float sylves_point_to_segment_distance_2d(
    const SylvesVector2* point,
    const SylvesVector2* seg_start,
    const SylvesVector2* seg_end,
    SylvesVector2* closest_out
);

/**
 * @brief Distance from point to line
 * 
 * @param point Query point
 * @param line_point Point on line
 * @param line_dir Line direction (normalized)
 * @return Distance to line
 */
float sylves_point_to_line_distance_2d(
    const SylvesVector2* point,
    const SylvesVector2* line_point,
    const SylvesVector2* line_dir
);

/* Area and perimeter */

/**
 * @brief Compute area of 2D polygon
 * 
 * Uses shoelace formula. Positive for CCW, negative for CW.
 * 
 * @param vertices Polygon vertices
 * @param num_vertices Number of vertices
 * @return Signed area
 */
float sylves_polygon_area_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
);

/**
 * @brief Compute perimeter of 2D polygon
 * 
 * @param vertices Polygon vertices
 * @param num_vertices Number of vertices
 * @return Perimeter length
 */
float sylves_polygon_perimeter_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
);

/**
 * @brief Compute centroid of 2D polygon
 * 
 * @param vertices Polygon vertices
 * @param num_vertices Number of vertices
 * @return Centroid point
 */
SylvesVector2 sylves_polygon_centroid_2d(
    const SylvesVector2* vertices,
    size_t num_vertices
);

/* Bounding boxes */

/**
 * @brief Compute 2D bounding box
 * 
 * @param points Input points
 * @param num_points Number of points
 * @param min_out Minimum corner
 * @param max_out Maximum corner
 */
void sylves_compute_bbox_2d(
    const SylvesVector2* points,
    size_t num_points,
    SylvesVector2* min_out,
    SylvesVector2* max_out
);

/**
 * @brief Compute 3D bounding box
 * 
 * @param points Input points
 * @param num_points Number of points
 * @param min_out Minimum corner
 * @param max_out Maximum corner
 */
void sylves_compute_bbox_3d(
    const SylvesVector3* points,
    size_t num_points,
    SylvesVector3* min_out,
    SylvesVector3* max_out
);

/* Triangulation */

/**
 * @brief Triangulate simple polygon
 * 
 * Uses ear clipping algorithm.
 * 
 * @param vertices Polygon vertices (CCW order)
 * @param num_vertices Number of vertices
 * @param triangles_out Array for triangle indices (3 per triangle)
 * @param max_triangles Maximum triangles that can be stored
 * @param num_triangles_out Number of triangles created
 * @return true on success
 */
bool sylves_triangulate_polygon_2d(
    const SylvesVector2* vertices,
    size_t num_vertices,
    int* triangles_out,
    size_t max_triangles,
    size_t* num_triangles_out
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_GEOMETRY_UTILS_H */
