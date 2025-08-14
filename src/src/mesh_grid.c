/**
 * @file mesh_grid.c
 * @brief Implementation of mesh-based grids
 */

#include "sylves/mesh.h"
#include "sylves/grid.h"
#include "sylves/vector.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/utils.h"
#include "internal/grid_internal.h"
#include "internal/grid_defaults.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    SylvesGrid base;
    SylvesMeshData* mesh;
    bool owns_mesh;  /* Whether we should free the mesh data */
} MeshGrid;

/* Forward declarations */
static void mesh_grid_destroy(SylvesGrid* grid);
static bool mesh_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static const SylvesCellType* mesh_grid_get_cell_type(const SylvesGrid* grid, SylvesCell cell);
static bool mesh_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                               SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static int mesh_grid_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                                   SylvesCellDir* dirs, size_t max_dirs);
static SylvesVector3 mesh_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static int mesh_grid_get_polygon(const SylvesGrid* grid, SylvesCell cell, 
                                 SylvesVector3* vertices, size_t max_vertices);
static bool mesh_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

/* VTable */
static const SylvesGridVTable mesh_grid_vtable = {
    .destroy = mesh_grid_destroy,
    .is_2d = NULL,
    .is_3d = NULL,
    .is_planar = NULL,
    .is_repeating = NULL,
    .is_orientable = NULL,
    .is_finite = NULL,
    .get_coordinate_dimension = NULL,
    .is_cell_in_grid = mesh_grid_is_cell_in_grid,
    .get_cell_type = mesh_grid_get_cell_type,
    .try_move = mesh_grid_try_move,
    .get_cell_dirs = mesh_grid_get_cell_dirs,
    .get_cell_corners = NULL,
    .get_cell_center = mesh_grid_get_cell_center,
    .get_cell_corner_pos = NULL,
    .get_polygon = mesh_grid_get_polygon,
    .get_cell_aabb = NULL,
    .find_cell = mesh_grid_find_cell,
    .raycast = NULL,
    .get_index_count = NULL,
    .get_index = NULL,
    .get_cell_by_index = NULL
};

/* Helper functions */
static void mesh_grid_destroy(SylvesGrid* grid) {
    if (grid && grid->data) {
        MeshGrid* mg = (MeshGrid*)grid->data;
        if (mg->owns_mesh && mg->mesh) {
            sylves_mesh_data_destroy(mg->mesh);
        }
        sylves_free(mg);
        sylves_free(grid);
    }
}


static bool mesh_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    return mg->mesh && cell.x >= 0 && cell.x < (int)(mg->mesh->face_count) && 
           cell.y == 0 && cell.z == 0;
}

static const SylvesCellType* mesh_grid_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    if (!mesh_grid_is_cell_in_grid(grid, cell)) {
        return NULL;
    }
    
    /* TODO: Return appropriate cell type based on face vertex count */
    /* For now, return NULL to indicate variable cell types */
    return NULL;
}

static bool mesh_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                               SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    if (!mesh_grid_is_cell_in_grid(grid, cell)) {
        return false;
    }
    
    SylvesMeshFace* face = &mg->mesh->faces[cell.x];
    
    if (dir < 0 || dir >= face->vertex_count) {
        return false;
    }
    
    int neighbor_idx = face->neighbors[dir];
    if (neighbor_idx < 0) {
        return false;
    }
    
    /* Find which edge we came from in the neighbor */
    SylvesMeshFace* neighbor_face = &mg->mesh->faces[neighbor_idx];
    int inv_dir = -1;
    for (int i = 0; i < neighbor_face->vertex_count; i++) {
        if (neighbor_face->neighbors[i] == cell.x) {
            inv_dir = i;
            break;
        }
    }
    
    if (inv_dir < 0) {
        return false; /* Inconsistent mesh topology */
    }
    
    if (dest) *dest = (SylvesCell){neighbor_idx, 0, 0};
    if (inverse_dir) *inverse_dir = inv_dir;
    if (connection) {
        connection->rotation = 0; /* TODO: Calculate proper rotation */
        connection->is_mirror = false;
    }
    
    return true;
}

static int mesh_grid_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                                   SylvesCellDir* dirs, size_t max_dirs) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    if (!mesh_grid_is_cell_in_grid(grid, cell)) {
        return 0;
    }
    
    SylvesMeshFace* face = &mg->mesh->faces[cell.x];
    
    int count = face->vertex_count;
    if (dirs && max_dirs > 0) {
        int n = count < max_dirs ? count : max_dirs;
        for (int i = 0; i < n; i++) {
            dirs[i] = i;
        }
    }
    
    return count;
}

static SylvesVector3 mesh_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    if (!mesh_grid_is_cell_in_grid(grid, cell)) {
        return (SylvesVector3){0, 0, 0};
    }
    
    SylvesMeshFace* face = &mg->mesh->faces[cell.x];
    
    /* Calculate centroid of face vertices */
    SylvesVector3 sum = {0, 0, 0};
    for (int i = 0; i < face->vertex_count; i++) {
        SylvesVector3 v = mg->mesh->vertices[face->vertices[i]];
        sum = sylves_vector3_add(sum, v);
    }
    
    return sylves_vector3_scale(sum, 1.0 / face->vertex_count);
}

static int mesh_grid_get_polygon(const SylvesGrid* grid, SylvesCell cell, 
                                 SylvesVector3* vertices, size_t max_vertices) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    if (!mesh_grid_is_cell_in_grid(grid, cell)) {
        return -1;  /* Invalid cell */
    }
    
    SylvesMeshFace* face = &mg->mesh->faces[cell.x];
    
    /* Copy vertices up to max_vertices */
    int count = face->vertex_count;
    if (vertices && max_vertices > 0) {
        int n = count < max_vertices ? count : max_vertices;
        for (int i = 0; i < n; i++) {
            vertices[i] = mg->mesh->vertices[face->vertices[i]];
        }
    }
    
    return count;
}

static bool mesh_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    const MeshGrid* mg = (const MeshGrid*)grid->data;
    
    /* Simple brute force search - check each face */
    /* TODO: Implement spatial acceleration structure */
    double min_dist = INFINITY;
    int closest_face = -1;
    
    for (int i = 0; i < (int)mg->mesh->face_count; i++) {
        SylvesCell test_cell = {i, 0, 0};
        SylvesVector3 center = mesh_grid_get_cell_center(grid, test_cell);
        
        double dist = sylves_vector3_distance(position, center);
        if (dist < min_dist) {
            min_dist = dist;
            closest_face = i;
        }
    }
    
    if (closest_face >= 0) {
        *cell = (SylvesCell){closest_face, 0, 0};
        return true;
    }
    
    return false;
}

/* Mesh data management */
SylvesMeshData* sylves_mesh_data_create(size_t vertex_count, size_t face_count) {
    if (vertex_count <= 0 || face_count <= 0) {
        return NULL;
    }
    
    SylvesMeshData* mesh = sylves_alloc(sizeof(SylvesMeshData));
    if (!mesh) {
        return NULL;
    }
    
    mesh->vertices = sylves_alloc(sizeof(SylvesVector3) * vertex_count);
    mesh->faces = sylves_alloc(sizeof(SylvesMeshFace) * face_count);
    
    if (!mesh->vertices || !mesh->faces) {
        sylves_free(mesh->vertices);
        sylves_free(mesh->faces);
        sylves_free(mesh);
        return NULL;
    }
    
    mesh->vertex_count = vertex_count;
    mesh->face_count = face_count;
    
    /* Initialize faces */
    for (int i = 0; i < face_count; i++) {
        mesh->faces[i].vertices = NULL;
        mesh->faces[i].vertex_count = 0;
        mesh->faces[i].neighbors = NULL;
    }
    
    return mesh;
}

void sylves_mesh_data_destroy(SylvesMeshData* mesh_data) {
    if (!mesh_data) return;
    
    /* Free face data */
    for (int i = 0; i < mesh_data->face_count; i++) {
        sylves_free(mesh_data->faces[i].vertices);
        sylves_free(mesh_data->faces[i].neighbors);
    }
    
    sylves_free(mesh_data->vertices);
    sylves_free(mesh_data->faces);
    sylves_free(mesh_data);
}

/* Mesh validation */
bool sylves_mesh_validate(const SylvesMeshData* mesh_data) {
    if (!mesh_data || !mesh_data->vertices || !mesh_data->faces) {
        return false;
    }
    
    /* Check each face */
    for (int i = 0; i < mesh_data->face_count; i++) {
        SylvesMeshFace* face = &mesh_data->faces[i];
        
        if (face->vertex_count < 3) {
            return false; /* Face must have at least 3 vertices */
        }
        
        if (!face->vertices || !face->neighbors) {
            return false;
        }
        
        /* Check vertex indices */
        for (int j = 0; j < face->vertex_count; j++) {
            if (face->vertices[j] < 0 || face->vertices[j] >= mesh_data->vertex_count) {
                return false;
            }
            
            /* Check neighbor indices */
            int neighbor = face->neighbors[j];
            if (neighbor < -1 || neighbor >= mesh_data->face_count) {
                return false;
            }
        }
    }
    
    return true;
}

bool sylves_mesh_is_manifold(const SylvesMeshData* mesh_data) {
    if (!sylves_mesh_validate(mesh_data)) {
        return false;
    }
    
    /* Check that each edge is shared by at most 2 faces */
    /* TODO: Implement proper manifold check */
    return true;
}

bool sylves_mesh_is_closed(const SylvesMeshData* mesh_data) {
    if (!sylves_mesh_validate(mesh_data)) {
        return false;
    }
    
    /* Check that there are no boundary edges */
    for (int i = 0; i < mesh_data->face_count; i++) {
        SylvesMeshFace* face = &mesh_data->faces[i];
        for (int j = 0; j < face->vertex_count; j++) {
            if (face->neighbors[j] < 0) {
                return false; /* Found boundary edge */
            }
        }
    }
    
    return true;
}

/* Mesh building helpers */
int sylves_mesh_data_add_ngon_face(SylvesMeshData* mesh_data,
                                   const SylvesVector3* vertices, 
                                   const int* indices, int n) {
    if (!mesh_data || !vertices || !indices || n < 3) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Find the next available face slot */
    int face_idx = -1;
    for (int i = 0; i < mesh_data->face_count; i++) {
        if (mesh_data->faces[i].vertex_count == 0) {
            face_idx = i;
            break;
        }
    }
    
    if (face_idx < 0) {
        return SYLVES_ERROR_OUT_OF_BOUNDS; /* No available face slots */
    }
    
    /* Add vertices to mesh if needed (simple approach: always add) */
    int base_vertex_idx = 0;
    for (int i = 0; i < mesh_data->vertex_count; i++) {
        /* Find first unused vertex slot */
        if (mesh_data->vertices[i].x == 0 && 
            mesh_data->vertices[i].y == 0 && 
            mesh_data->vertices[i].z == 0) {
            base_vertex_idx = i;
            break;
        }
    }
    
    /* Check if we have enough vertex slots */
    if (base_vertex_idx + n > mesh_data->vertex_count) {
        /* For now, use provided indices directly instead of adding new vertices */
        base_vertex_idx = -1;
    }
    
    /* Allocate face arrays */
    SylvesMeshFace* face = &mesh_data->faces[face_idx];
    face->vertices = sylves_alloc(sizeof(int) * n);
    face->neighbors = sylves_alloc(sizeof(int) * n);
    
    if (!face->vertices || !face->neighbors) {
        sylves_free(face->vertices);
        sylves_free(face->neighbors);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    face->vertex_count = n;
    
    /* Set vertex indices and copy vertices if needed */
    if (base_vertex_idx >= 0) {
        /* Add new vertices */
        for (int i = 0; i < n; i++) {
            mesh_data->vertices[base_vertex_idx + i] = vertices[i];
            face->vertices[i] = base_vertex_idx + i;
        }
    } else {
        /* Use provided indices */
        for (int i = 0; i < n; i++) {
            /* Ensure indices are valid */
            if (indices[i] < 0 || indices[i] >= mesh_data->vertex_count) {
                /* Copy vertex directly if index refers to input array */
                if (indices[i] >= 0 && indices[i] < n) {
                    /* Find or add vertex */
                    int found_idx = -1;
                    for (int j = 0; j < mesh_data->vertex_count; j++) {
                        if (fabs(mesh_data->vertices[j].x - vertices[indices[i]].x) < 1e-6 &&
                            fabs(mesh_data->vertices[j].y - vertices[indices[i]].y) < 1e-6 &&
                            fabs(mesh_data->vertices[j].z - vertices[indices[i]].z) < 1e-6) {
                            found_idx = j;
                            break;
                        }
                    }
                    if (found_idx >= 0) {
                        face->vertices[i] = found_idx;
                    } else {
                        /* Add vertex if space available */
                        for (int j = 0; j < mesh_data->vertex_count; j++) {
                            if (mesh_data->vertices[j].x == 0 && 
                                mesh_data->vertices[j].y == 0 && 
                                mesh_data->vertices[j].z == 0) {
                                mesh_data->vertices[j] = vertices[indices[i]];
                                face->vertices[i] = j;
                                break;
                            }
                        }
                    }
                } else {
                    face->vertices[i] = indices[i];
                }
            } else {
                face->vertices[i] = indices[i];
            }
        }
    }
    
    /* Initialize neighbors to -1 (boundary) */
    for (int i = 0; i < n; i++) {
        face->neighbors[i] = -1;
    }
    
    return SYLVES_SUCCESS;
}

/* Mesh utilities */
int sylves_mesh_compute_adjacency(SylvesMeshData* mesh_data) {
    if (!mesh_data) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Build edge-to-face map to compute adjacency */
    /* For each face, check all edges against all other faces */
    for (int i = 0; i < mesh_data->face_count; i++) {
        SylvesMeshFace* face1 = &mesh_data->faces[i];
        if (face1->vertex_count == 0) continue;
        
        for (int edge1 = 0; edge1 < face1->vertex_count; edge1++) {
            if (face1->neighbors[edge1] >= 0) continue; /* Already has neighbor */
            
            int v1 = face1->vertices[edge1];
            int v2 = face1->vertices[(edge1 + 1) % face1->vertex_count];
            
            /* Search for matching edge in other faces */
            for (int j = 0; j < mesh_data->face_count; j++) {
                if (i == j) continue;
                
                SylvesMeshFace* face2 = &mesh_data->faces[j];
                if (face2->vertex_count == 0) continue;
                
                for (int edge2 = 0; edge2 < face2->vertex_count; edge2++) {
                    int v3 = face2->vertices[edge2];
                    int v4 = face2->vertices[(edge2 + 1) % face2->vertex_count];
                    
                    /* Check if edges match (opposite direction) */
                    if ((v1 == v4 && v2 == v3)) {
                        face1->neighbors[edge1] = j;
                        face2->neighbors[edge2] = i;
                        break;
                    }
                }
            }
        }
    }
    
    return SYLVES_SUCCESS;
}

int sylves_mesh_orient_consistently(SylvesMeshData* mesh_data) {
    if (!mesh_data) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Orient all faces consistently */
    /* TODO: Implement consistent orientation */
    return SYLVES_SUCCESS;
}

/* Creation functions */
SylvesGrid* sylves_mesh_grid_create(const SylvesMeshData* mesh_data) {
    if (!mesh_data || !sylves_mesh_validate(mesh_data)) {
        return NULL;
    }
    
    MeshGrid* mg = sylves_alloc(sizeof(MeshGrid));
    if (!mg) {
        return NULL;
    }
    
    /* Create a copy of the mesh data */
    SylvesMeshData* mesh_copy = sylves_mesh_data_create(mesh_data->vertex_count, mesh_data->face_count);
    if (!mesh_copy) {
        sylves_free(mg);
        return NULL;
    }
    
    /* Copy vertices */
    memcpy(mesh_copy->vertices, mesh_data->vertices, sizeof(SylvesVector3) * mesh_data->vertex_count);
    
    /* Copy faces */
    for (int i = 0; i < mesh_data->face_count; i++) {
        SylvesMeshFace* src = &mesh_data->faces[i];
        SylvesMeshFace* dst = &mesh_copy->faces[i];
        
        dst->vertex_count = src->vertex_count;
        dst->vertices = sylves_alloc(sizeof(int) * src->vertex_count);
        dst->neighbors = sylves_alloc(sizeof(int) * src->vertex_count);
        
        if (!dst->vertices || !dst->neighbors) {
            sylves_mesh_data_destroy(mesh_copy);
            sylves_free(mg);
            return NULL;
        }
        
        memcpy(dst->vertices, src->vertices, sizeof(int) * src->vertex_count);
        memcpy(dst->neighbors, src->neighbors, sizeof(int) * src->vertex_count);
    }
    
    /* Initialize base grid */
    SylvesGrid* grid = sylves_alloc(sizeof(SylvesGrid));
    if (!grid) {
        sylves_mesh_data_destroy(mesh_copy);
        sylves_free(mg);
        return NULL;
    }
    
    grid->vtable = &mesh_grid_vtable;
    grid->type = SYLVES_GRID_TYPE_MESH;
    grid->bound = NULL;
    grid->data = mg;
    
    mg->base = *grid;  /* Copy base grid info */
    mg->mesh = mesh_copy;
    mg->owns_mesh = true;
    
    return grid;
}

SylvesGrid* sylves_mesh_grid_create_from_arrays(
    const SylvesVector3* vertices, int vertex_count,
    const int* face_indices, const int* face_sizes, int face_count) {
    
    if (!vertices || !face_indices || !face_sizes || 
        vertex_count <= 0 || face_count <= 0) {
        return NULL;
    }
    
    /* Create mesh data */
    SylvesMeshData* mesh = sylves_mesh_data_create(vertex_count, face_count);
    if (!mesh) {
        return NULL;
    }
    
    /* Copy vertices */
    memcpy(mesh->vertices, vertices, sizeof(SylvesVector3) * vertex_count);
    
    /* Build faces */
    int index_offset = 0;
    for (int i = 0; i < face_count; i++) {
        int face_size = face_sizes[i];
        if (face_size < 3) {
            sylves_mesh_data_destroy(mesh);
            return NULL;
        }
        
        mesh->faces[i].vertex_count = face_size;
        mesh->faces[i].vertices = sylves_alloc(sizeof(int) * face_size);
        mesh->faces[i].neighbors = sylves_alloc(sizeof(int) * face_size);
        
        if (!mesh->faces[i].vertices || !mesh->faces[i].neighbors) {
            sylves_mesh_data_destroy(mesh);
            return NULL;
        }
        
        /* Copy vertex indices */
        memcpy(mesh->faces[i].vertices, &face_indices[index_offset], sizeof(int) * face_size);
        
        /* Initialize neighbors to -1 (boundary) */
        for (int j = 0; j < face_size; j++) {
            mesh->faces[i].neighbors[j] = -1;
        }
        
        index_offset += face_size;
    }
    
    /* Compute adjacency */
    sylves_mesh_compute_adjacency(mesh);
    
    /* Create grid */
    SylvesGrid* grid = sylves_mesh_grid_create(mesh);
    sylves_mesh_data_destroy(mesh); /* Grid made its own copy */
    
    return grid;
}
