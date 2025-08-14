#include "sylves/voronoi.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static bool sylves_voronoi_get_polygon(const SylvesVoronoi* voronoi, int i, SylvesVector2* vertices_out, int max_vertices, int* num_vertices_out);

SylvesVoronoi* sylves_voronoi_create(
    SylvesDelaunay* delaunay,
    const SylvesVector2* bounds_min,
    const SylvesVector2* bounds_max,
    SylvesError* error_out
) {
    SylvesVoronoi* voronoi = malloc(sizeof(SylvesVoronoi));
    if (!voronoi) {
        if (error_out) {
*error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }

    voronoi->delaunay = delaunay;
    voronoi->circumcenters = malloc(delaunay->num_triangles * sizeof(SylvesVector2));
    if (!voronoi->circumcenters) {
        free(voronoi);
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    // Compute circumcenters
    sylves_compute_circumcenters(delaunay, voronoi->circumcenters);

    // Allocate inedges array
    voronoi->inedges = malloc(delaunay->num_points * sizeof(int));
    if (!voronoi->inedges) {
        free(voronoi->circumcenters);
        free(voronoi);
        if (error_out) {
*error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    memset(voronoi->inedges, -1, delaunay->num_points * sizeof(int));

    for (int e = 0; e < delaunay->num_triangles * 3; ++e) {
        if (delaunay->halfedges[e] == -1 || voronoi->inedges[delaunay->triangles[sylves_delaunay_next_halfedge(e)]] == -1) {
            voronoi->inedges[delaunay->triangles[sylves_delaunay_next_halfedge(e)]] = e;
        }
    }

    if (bounds_min && bounds_max) {
        voronoi->bounds_min = *bounds_min;
        voronoi->bounds_max = *bounds_max;
    } else {
        voronoi->bounds_min = (SylvesVector2){-INFINITY, -INFINITY};
        voronoi->bounds_max = (SylvesVector2){INFINITY, INFINITY};
    }

    if (error_out) {
        *error_out = SYLVES_SUCCESS;
    }

    return voronoi;
}

void sylves_voronoi_destroy(SylvesVoronoi* voronoi) {
    if (!voronoi) return;

    if (voronoi->circumcenters) free(voronoi->circumcenters);
    if (voronoi->inedges) free(voronoi->inedges);

    free(voronoi);
}

int sylves_voronoi_get_cell(
    const SylvesVoronoi* voronoi,
    int point_index,
    SylvesVector2* vertices_out,
    int max_vertices
) {
    int num_vertices;
    if (sylves_voronoi_get_polygon(voronoi, point_index, vertices_out, max_vertices, &num_vertices)) {
        return num_vertices;
    } else {
        return 0;
    }
}

static bool sylves_voronoi_get_polygon(const SylvesVoronoi* voronoi, int i, SylvesVector2* vertices_out, int max_vertices, int* num_vertices_out) {
    int count = 0;

    int e0 = voronoi->inedges[i];
    if (e0 == -1) {
        *num_vertices_out = 0;
        return false;
    }

    int e = e0;
    do {
        if (count >= max_vertices) {
            *num_vertices_out = count;
            return false;
        }
        vertices_out[count++] = voronoi->circumcenters[sylves_delaunay_edge_to_triangle(e)];
        e = voronoi->delaunay->halfedges[sylves_delaunay_next_halfedge(e)];
    } while (e != e0 && e != -1);

    // For unbounded polygons, handle open edges
    if (e == -1) {
        // Add logic for handling open edges extending to infinity
    }

    *num_vertices_out = count;
    return true;
}

bool sylves_compute_circumcenters(
    const SylvesDelaunay* delaunay,
    SylvesVector2* circumcenters
) {
    for (int i = 0; i < delaunay->num_triangles; ++i) {
        int t = i * 3;
        sylves_circumcenter(
            delaunay->coords[delaunay->triangles[t] * 2],
            delaunay->coords[delaunay->triangles[t] * 2 + 1],
            delaunay->coords[delaunay->triangles[t + 1] * 2],
            delaunay->coords[delaunay->triangles[t + 1] * 2 + 1],
            delaunay->coords[delaunay->triangles[t + 2] * 2],
            delaunay->coords[delaunay->triangles[t + 2] * 2 + 1],
            &circumcenters[i].x,
            &circumcenters[i].y
        );
    }
    return true;
}

void sylves_circumcenter(
    float ax, float ay,
    float bx, float by,
    float cx, float cy,
    float* cx_out, float* cy_out
) {
    float dx = bx - ax;
    float dy = by - ay;
    float ex = cx - ax;
    float ey = cy - ay;
    float bl = dx * dx + dy * dy;
    float cl = ex * ex + ey * ey;
    float d = 0.5f / (dx * ey - dy * ex);
    *cx_out = ax + (ey * bl - dy * cl) * d;
    *cy_out = ay + (dx * cl - ex * bl) * d;
}
