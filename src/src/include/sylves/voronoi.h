/**
 * @file voronoi.h
 * @brief Voronoi diagram utilities
 */

#ifndef SYLVES_VORONOI_H
#define SYLVES_VORONOI_H

#include "types.h"
#include "delaunay.h"
#include "mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Voronoi diagram
 */
typedef struct SylvesVoronoi {
    SylvesDelaunay* delaunay; /** Associated Delaunay triangulation */
    SylvesVector2* circumcenters; /** Circumcenters of Delaunay triangles */
    int* inedges; /** Map from point index to half-edge */
    SylvesVector2 bounds_min, bounds_max; /** Clipping bounds */
} SylvesVoronoi;

/**
 * @brief Create Voronoi diagram
 *
 * The Voronoi diagram is constructed as the dual of a Delaunay triangulation.
 * Optionally, a bounding box can be provided for clipping purposes.
 *
 * @param delaunay Precomputed Delaunay triangulation (ownership is transferred)
 * @param bounds_min Minimum bounds for clipping (optional, pass NULL if unused)
 * @param bounds_max Maximum bounds for clipping (optional, pass NULL if unused)
 * @param error_out Optional error output
 * @return New Voronoi diagram or NULL on error
 */
SylvesVoronoi* sylves_voronoi_create(
    SylvesDelaunay* delaunay,
    const SylvesVector2* bounds_min,
    const SylvesVector2* bounds_max,
    SylvesError* error_out
);

/**
 * @brief Destroy Voronoi diagram
 * @param voronoi Voronoi diagram to destroy
 */
void sylves_voronoi_destroy(SylvesVoronoi* voronoi);

/**
 * @brief Get Voronoi cell polygon
 *
 * For bounded cells, the polygon will be closed. For unbounded cells, the 
 * polygon will include the edges extending to infinity.
 *
 * @param voronoi Voronoi diagram
 * @param point_index Index of point corresponding to cell
 * @param vertices_out Array to fill with cell vertices (must be large enough)
 * @param max_vertices Maximum number of vertices that can be stored
 * @return Number of vertices in cell polygon
 */
int sylves_voronoi_get_cell(
    const SylvesVoronoi* voronoi,
    int point_index,
    SylvesVector2* vertices_out,
    int max_vertices
);

/**
 * @brief Compute circumcenters for Delaunay triangulation
 * @param delaunay Delaunay triangulation
 * @param circumcenters Array to fill with circumcenters (must be large enough)
 * @return Success/failure
 */
bool sylves_compute_circumcenters(
    const SylvesDelaunay* delaunay,
    SylvesVector2* circumcenters
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_VORONOI_H */
