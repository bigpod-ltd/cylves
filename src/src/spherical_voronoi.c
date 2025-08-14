/**
 * @file spherical_voronoi.c
 * @brief Spherical Voronoi implementation following Sylves
 */

#include "sylves/spherical_voronoi.h"
#include "sylves/vector.h"
#include "sylves/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

SylvesSphericalVoronoi* sylves_spherical_voronoi_create(
    const SylvesVector3* points,
    size_t num_points,
    SylvesError* error_out
) {
    if (!points || num_points < 4) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }

    SylvesSphericalVoronoi* sv = calloc(1, sizeof(SylvesSphericalVoronoi));
    if (!sv) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Copy points */
    sv->points = malloc(num_points * sizeof(SylvesVector3));
    if (!sv->points) {
        sylves_spherical_voronoi_destroy(sv);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    memcpy(sv->points, points, num_points * sizeof(SylvesVector3));
    sv->num_points = num_points;

    /* Project points to 2D using stereographic projection */
    /* Last point is the projection pole */
    SylvesVector3 pole = sylves_vector3_normalize(points[num_points - 1]);
    SylvesVector3 u, v;
    sylves_get_plane_basis(&pole, &u, &v);

    /* Allocate 2D points (excluding pole) */
    SylvesVector2* points2d = malloc((num_points - 1) * sizeof(SylvesVector2));
    if (!points2d) {
        sylves_spherical_voronoi_destroy(sv);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Project all points except pole */
    for (size_t i = 0; i < num_points - 1; i++) {
        points2d[i] = sylves_stereographic_project(&points[i], &pole, &u, &v);
    }

    /* Create 2D Delaunay triangulation */
    sv->delaunay = sylves_delaunay_create(points2d, num_points - 1, error_out);
    free(points2d);
    
    if (!sv->delaunay) {
        sylves_spherical_voronoi_destroy(sv);
        return NULL;
    }

    /* Compute circumcenters on sphere */
    sv->circumcenters = malloc(sv->delaunay->num_triangles * sizeof(SylvesVector3));
    if (!sv->circumcenters) {
        sylves_spherical_voronoi_destroy(sv);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    for (size_t i = 0; i < sv->delaunay->num_triangles; i++) {
        int p0, p1, p2;
        sylves_delaunay_points_around_triangle(sv->delaunay, (int)i, &p0, &p1, &p2);
        sv->circumcenters[i] = sylves_spherical_circumcenter(
            &points[p0], &points[p1], &points[p2]
        );
    }

    /* Find starting half-edge for each point */
    sv->inedges = malloc(num_points * sizeof(int));
    if (!sv->inedges) {
        sylves_spherical_voronoi_destroy(sv);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    memset(sv->inedges, -1, num_points * sizeof(int));

    for (int e = 0; e < sv->delaunay->num_triangles * 3; e++) {
        int p = sv->delaunay->triangles[sylves_delaunay_next_halfedge(e)];
        if (sv->delaunay->halfedges[e] == -1 || sv->inedges[p] == -1) {
            sv->inedges[p] = e;
        }
    }

    /* Compute hull circumcenters */
    sv->hull_circumcenters = malloc(num_points * sizeof(SylvesVector3));
    if (!sv->hull_circumcenters) {
        sylves_spherical_voronoi_destroy(sv);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* For each hull edge, compute circumcenter with pole */
    int h = sv->delaunay->hull[sv->delaunay->hull_size - 1];
    for (size_t i = 0; i < sv->delaunay->hull_size; i++) {
        int next = sv->delaunay->hull[i];
        sv->hull_circumcenters[h] = sylves_spherical_circumcenter(
            &points[h], &points[next], &points[num_points - 1]
        );
        h = next;
    }

    if (error_out) *error_out = SYLVES_SUCCESS;
    return sv;
}

void sylves_spherical_voronoi_destroy(SylvesSphericalVoronoi* voronoi) {
    if (!voronoi) return;

    free(voronoi->points);
    if (voronoi->delaunay) sylves_delaunay_destroy(voronoi->delaunay);
    free(voronoi->circumcenters);
    free(voronoi->inedges);
    free(voronoi->hull_circumcenters);
    free(voronoi);
}

int sylves_spherical_voronoi_get_cell(
    const SylvesSphericalVoronoi* voronoi,
    int point_index,
    SylvesVector3* vertices_out,
    int max_vertices
) {
    if (!voronoi || point_index < 0 || point_index >= (int)voronoi->num_points - 1) {
        return 0;
    }

    int e0 = voronoi->inedges[point_index];
    if (e0 == -1) {
        return 0;
    }

    int count = 0;
    int e = e0;
    int prev_e;

    do {
        if (count >= max_vertices) {
            return count;
        }
        vertices_out[count++] = voronoi->circumcenters[sylves_delaunay_edge_to_triangle(e)];
        prev_e = sylves_delaunay_next_halfedge(e);
        e = voronoi->delaunay->halfedges[prev_e];
    } while (e != e0 && e != -1);

    /* Handle open polygon (cell extends to infinity/pole) */
    if (e == -1) {
        if (count < max_vertices) {
            vertices_out[count++] = voronoi->hull_circumcenters[voronoi->delaunay->triangles[prev_e]];
        }
        if (count < max_vertices) {
            vertices_out[count++] = voronoi->hull_circumcenters[voronoi->delaunay->triangles[e0]];
        }
    }

    return count;
}

SylvesMeshData* sylves_spherical_voronoi_create_mesh(
    const SylvesSphericalVoronoi* voronoi,
    SylvesError* error_out
) {
    /* Count total vertices needed */
    size_t total_vertices = 0;
    size_t total_indices = 0;
    
    /* Process each cell */
    for (size_t i = 0; i < voronoi->num_points; i++) {
        SylvesVector3 cell_verts[32];
        int n = sylves_spherical_voronoi_get_cell(voronoi, (int)i, cell_verts, 32);
        if (n > 0) {
            total_vertices += n;
            total_indices += n;
        }
    }

    /* Allocate mesh data */
    size_t face_count = voronoi->num_points;
    SylvesMeshData* mesh = sylves_mesh_data_create(total_vertices, face_count);
    if (!mesh) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Fill mesh data */
    size_t vertex_offset = 0;
    size_t face_idx = 0;

    /* First, copy all vertices */
    for (size_t i = 0; i < voronoi->num_points; i++) {
        SylvesVector3 cell_verts[32];
        int n = sylves_spherical_voronoi_get_cell(voronoi, (int)i, cell_verts, 32);
        
        if (n > 0) {
            /* Copy vertices for this face */
            memcpy(&mesh->vertices[vertex_offset], cell_verts, n * sizeof(SylvesVector3));
            
            /* Set up face data */
            mesh->faces[face_idx].vertex_count = n;
            mesh->faces[face_idx].vertices = sylves_alloc(sizeof(int) * n);
            mesh->faces[face_idx].neighbors = sylves_alloc(sizeof(int) * n);
            
            if (!mesh->faces[face_idx].vertices || !mesh->faces[face_idx].neighbors) {
                sylves_mesh_data_destroy(mesh);
                if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
                return NULL;
            }
            
            /* Set vertex indices for this face */
            for (int j = 0; j < n; j++) {
                mesh->faces[face_idx].vertices[j] = (int)(vertex_offset + j);
                mesh->faces[face_idx].neighbors[j] = -1; /* No neighbor info for now */
            }
            
            vertex_offset += n;
            face_idx++;
        }
    }

    return mesh;
}

/* Spherical geometry utilities */

SylvesVector2 sylves_stereographic_project(
    const SylvesVector3* p,
    const SylvesVector3* pole,
    const SylvesVector3* u,
    const SylvesVector3* v
) {
    float denom = 1.0f - sylves_vector3_dot(*pole, *p);
    if (fabsf(denom) < 1e-6f) {
        /* Point is at pole - project to infinity */
        return (SylvesVector2){INFINITY, INFINITY};
    }

    /* proj3D = pole + (p - pole) / denom */
    SylvesVector3 diff = sylves_vector3_subtract(*p, *pole);
    SylvesVector3 proj3d = sylves_vector3_add(
        *pole,
        sylves_vector3_scale(diff, 1.0f / denom)
    );

    /* Project onto u,v basis */
    return (SylvesVector2){
        sylves_vector3_dot(proj3d, *u),
        sylves_vector3_dot(proj3d, *v)
    };
}

void sylves_get_plane_basis(
    const SylvesVector3* normal,
    SylvesVector3* u,
    SylvesVector3* v
) {
    SylvesVector3 n = sylves_vector3_normalize(*normal);
    
    /* Pick arbitrary vector not parallel to normal */
    SylvesVector3 temp;
    SylvesVector3 up = {0, 1, 0};
    SylvesVector3 right = {1, 0, 0};
    if (fabsf(sylves_vector3_dot(n, up)) < 0.99f) {
        temp = up;
    } else {
        temp = right;
    }

    /* Create orthonormal basis */
    SylvesVector3 cross_product = sylves_vector3_cross(n, temp);
    *u = sylves_vector3_normalize(cross_product);
    *v = sylves_vector3_cross(n, *u);
}

SylvesVector3 sylves_spherical_circumcenter(
    const SylvesVector3* a,
    const SylvesVector3* b,
    const SylvesVector3* c
) {
    /* Following Sylves algorithm */
    SylvesVector3 ac = sylves_vector3_subtract(*c, *a);
    SylvesVector3 ab = sylves_vector3_subtract(*b, *a);
    SylvesVector3 ab_cross_ac = sylves_vector3_cross(ab, ac);

    float ac_sq = sylves_vector3_length_squared(ac);
    float ab_sq = sylves_vector3_length_squared(ab);
    float cross_sq = sylves_vector3_length_squared(ab_cross_ac);

    /* Vector from a to circumsphere center */
    SylvesVector3 cross_ab = sylves_vector3_cross(ab_cross_ac, ab);
    SylvesVector3 cross_ac = sylves_vector3_cross(ac, ab_cross_ac);
    
    SylvesVector3 scaled_ab = sylves_vector3_scale(cross_ab, ac_sq);
    SylvesVector3 scaled_ac = sylves_vector3_scale(cross_ac, ab_sq);
    SylvesVector3 to_center = sylves_vector3_add(scaled_ab, scaled_ac);
    to_center = sylves_vector3_scale(to_center, 1.0f / (2.0f * cross_sq));

    /* Circumsphere center */
    SylvesVector3 center = sylves_vector3_add(*a, to_center);
    
    /* Normalize to sphere surface */
    return sylves_vector3_normalize(center);
}
