/**
 * @file spherical_voronoi.h
 * @brief Spherical Voronoi diagram on unit sphere
 */

#ifndef SYLVES_SPHERICAL_VORONOI_H
#define SYLVES_SPHERICAL_VORONOI_H

#include "types.h"
#include "mesh.h"
#include "delaunay.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Spherical Voronoi diagram structure
 */
typedef struct SylvesSphericalVoronoi {
    SylvesVector3* points;          /**< Input points on sphere */
    size_t num_points;              /**< Number of points */
    SylvesDelaunay* delaunay;       /**< 2D Delaunay triangulation */
    SylvesVector3* circumcenters;   /**< Circumcenters on sphere */
    int* inedges;                   /**< Map from point to half-edge */
    SylvesVector3* hull_circumcenters; /**< Circumcenters for hull edges */
} SylvesSphericalVoronoi;

/**
 * @brief Create spherical Voronoi diagram
 * 
 * Points should be normalized to lie on the unit sphere.
 * The last point is treated as the projection pole.
 * 
 * @param points Array of points on unit sphere
 * @param num_points Number of points (including pole)
 * @param error_out Optional error output
 * @return New spherical Voronoi or NULL on error
 */
SylvesSphericalVoronoi* sylves_spherical_voronoi_create(
    const SylvesVector3* points,
    size_t num_points,
    SylvesError* error_out
);

/**
 * @brief Destroy spherical Voronoi diagram
 * @param voronoi Spherical Voronoi to destroy
 */
void sylves_spherical_voronoi_destroy(SylvesSphericalVoronoi* voronoi);

/**
 * @brief Get Voronoi cell polygon on sphere
 * @param voronoi Spherical Voronoi diagram
 * @param point_index Index of point (excluding pole)
 * @param vertices_out Array to fill with cell vertices
 * @param max_vertices Maximum vertices that can be stored
 * @return Number of vertices in cell
 */
int sylves_spherical_voronoi_get_cell(
    const SylvesSphericalVoronoi* voronoi,
    int point_index,
    SylvesVector3* vertices_out,
    int max_vertices
);

/**
 * @brief Create mesh data from spherical Voronoi
 * @param voronoi Spherical Voronoi diagram
 * @param error_out Optional error output
 * @return New mesh data or NULL on error
 */
SylvesMeshData* sylves_spherical_voronoi_create_mesh(
    const SylvesSphericalVoronoi* voronoi,
    SylvesError* error_out
);

/* Spherical geometry utilities */

/**
 * @brief Stereographic projection from sphere to plane
 * @param p Point on sphere to project
 * @param pole Projection pole (normalized)
 * @param u First basis vector in plane
 * @param v Second basis vector in plane
 * @return Projected 2D point
 */
SylvesVector2 sylves_stereographic_project(
    const SylvesVector3* p,
    const SylvesVector3* pole,
    const SylvesVector3* u,
    const SylvesVector3* v
);

/**
 * @brief Get orthonormal basis for plane perpendicular to vector
 * @param normal Normal vector (will be normalized)
 * @param u Output first basis vector
 * @param v Output second basis vector
 */
void sylves_get_plane_basis(
    const SylvesVector3* normal,
    SylvesVector3* u,
    SylvesVector3* v
);

/**
 * @brief Compute circumcenter of spherical triangle
 * @param a First vertex (on sphere)
 * @param b Second vertex (on sphere)
 * @param c Third vertex (on sphere)
 * @return Circumcenter (normalized to sphere)
 */
SylvesVector3 sylves_spherical_circumcenter(
    const SylvesVector3* a,
    const SylvesVector3* b,
    const SylvesVector3* c
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_SPHERICAL_VORONOI_H */
