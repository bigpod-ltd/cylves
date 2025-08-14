/**
 * @file mesh.h
 * @brief Mesh grid - arbitrary mesh-based grids
 */

#ifndef SYLVES_MESH_H
#define SYLVES_MESH_H

#include "types.h"
#include "grid.h"
#include "vector.h"

/* SylvesMeshFace and SylvesMeshData are defined in types.h */

/* Creation */
SylvesGrid* sylves_mesh_grid_create(const SylvesMeshData* mesh_data);
SylvesGrid* sylves_mesh_grid_create_from_arrays(
    const SylvesVector3* vertices, int vertex_count,
    const int* face_indices, const int* face_sizes, int face_count);

/* Mesh data management */
SylvesMeshData* sylves_mesh_data_create(size_t vertex_count, size_t face_count);
void sylves_mesh_data_destroy(SylvesMeshData* mesh_data);

/* Mesh validation */
bool sylves_mesh_validate(const SylvesMeshData* mesh_data);
bool sylves_mesh_is_manifold(const SylvesMeshData* mesh_data);
bool sylves_mesh_is_closed(const SylvesMeshData* mesh_data);

/* Mesh building helpers */
int sylves_mesh_data_add_ngon_face(SylvesMeshData* mesh_data,
                                   const SylvesVector3* vertices, 
                                   const int* indices, int n);

/* Mesh utilities */
int sylves_mesh_compute_adjacency(SylvesMeshData* mesh_data);
int sylves_mesh_orient_consistently(SylvesMeshData* mesh_data);

#endif /* SYLVES_MESH_H */
