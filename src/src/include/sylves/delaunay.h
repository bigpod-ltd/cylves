/**
 * @file delaunay.h
 * @brief Delaunay triangulation implementation following Sylves
 */

#ifndef SYLVES_DELAUNAY_H
#define SYLVES_DELAUNAY_H

#include "types.h"
#include "errors.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delaunay triangle structure
 */
typedef struct SylvesDelaunayTriangle {
    int index;              /**< Triangle index */
    int p0, p1, p2;        /**< Point indices */
} SylvesDelaunayTriangle;

/**
 * @brief Delaunay triangulation structure
 */
typedef struct SylvesDelaunay {
    /* Input points */
    const SylvesVector2* points;
    size_t num_points;
    
    /* Triangulation data */
    int* triangles;        /**< One value per half-edge, containing point index where half edge starts */
    int* halfedges;        /**< One value per half-edge, containing opposite half-edge or -1 */
    size_t num_triangles;  /**< Number of triangles */
    
    /* Hull data */
    int* hull;             /**< Point indices that traverse the hull */
    size_t hull_size;      /**< Number of hull points */
    
    /* Internal data */
    float* coords;         /**< Flattened coordinates for performance */
    int* hull_prev;        /**< Previous hull edge */
    int* hull_next;        /**< Next hull edge */  
    int* hull_tri;         /**< Triangle associated with hull edge */
    int* hull_hash;        /**< Hash table for hull lookups */
    int hash_size;         /**< Size of hash table */
    
    /* Circumcenter of initial triangle */
    float cx, cy;
    
    /* Hull start for legalize function */
    int hull_start;
    
    /* Allocation info */
    size_t triangles_capacity;
} SylvesDelaunay;

/**
 * @brief Create Delaunay triangulation from points
 * @param points Array of 2D points
 * @param num_points Number of points
 * @param error_out Optional error output
 * @return New Delaunay triangulation or NULL on error
 */
SylvesDelaunay* sylves_delaunay_create(
    const SylvesVector2* points,
    size_t num_points,
    SylvesError* error_out
);

/**
 * @brief Destroy Delaunay triangulation
 * @param delaunay Delaunay triangulation to destroy
 */
void sylves_delaunay_destroy(SylvesDelaunay* delaunay);

/**
 * @brief Get triangle by index
 * @param delaunay Delaunay triangulation
 * @param triangle_index Triangle index
 * @param triangle_out Output triangle
 * @return Success/failure
 */
bool sylves_delaunay_get_triangle(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    SylvesDelaunayTriangle* triangle_out
);

/**
 * @brief Get all triangles
 * @param delaunay Delaunay triangulation
 * @param triangles_out Array to fill with triangles (must be large enough)
 * @param num_triangles_out Number of triangles written
 * @return Success/failure
 */
bool sylves_delaunay_get_triangles(
    const SylvesDelaunay* delaunay,
    SylvesDelaunayTriangle* triangles_out,
    size_t* num_triangles_out
);

/**
 * @brief Get edge by half-edge index
 * @param delaunay Delaunay triangulation
 * @param edge_index Half-edge index
 * @param p0_out First point of edge
 * @param p1_out Second point of edge
 * @return Success/failure
 */
bool sylves_delaunay_get_edge(
    const SylvesDelaunay* delaunay,
    int edge_index,
    SylvesVector2* p0_out,
    SylvesVector2* p1_out
);

/**
 * @brief Get circumcenter of triangle
 * @param delaunay Delaunay triangulation
 * @param triangle_index Triangle index
 * @param circumcenter_out Output circumcenter
 * @return Success/failure
 */
bool sylves_delaunay_get_triangle_circumcenter(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    SylvesVector2* circumcenter_out
);

/**
 * @brief Get points around triangle
 * @param delaunay Delaunay triangulation
 * @param triangle_index Triangle index
 * @param p0 Output index of first point
 * @param p1 Output index of second point
 * @param p2 Output index of third point
 * @return Success/failure
 */
bool sylves_delaunay_points_around_triangle(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    int* p0,
    int* p1,
    int* p2
);

/* Geometric predicates */

/**
 * @brief Robust orientation test (CCW test)
 * @return true if points are in counter-clockwise order
 */
bool sylves_orient2d(
    double px, double py,
    double qx, double qy,
    double rx, double ry
);

/**
 * @brief Robust in-circle test
 * @return true if point (px,py) is inside circumcircle of triangle (ax,ay), (bx,by), (cx,cy)
 */
bool sylves_incircle(
    double ax, double ay,
    double bx, double by,
    double cx, double cy,
    double px, double py
);

/**
 * @brief Compute circumradius of triangle
 */
double sylves_circumradius(
    double ax, double ay,
    double bx, double by,
    double cx, double cy
);

/**
 * @brief Compute circumcenter of triangle
 */
void sylves_circumcenter(
    float ax, float ay,
    float bx, float by,
    float cx, float cy,
    float* cx_out, float* cy_out
);

/* Helper functions */

/**
 * @brief Get next half-edge in triangle
 */
static inline int sylves_delaunay_next_halfedge(int e) {
    return (e % 3 == 2) ? e - 2 : e + 1;
}

/**
 * @brief Get previous half-edge in triangle  
 */
static inline int sylves_delaunay_prev_halfedge(int e) {
    return (e % 3 == 0) ? e + 2 : e - 1;
}

/**
 * @brief Convert edge index to triangle index
 */
static inline int sylves_delaunay_edge_to_triangle(int e) {
    return e / 3;
}

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_DELAUNAY_H */
