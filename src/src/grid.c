/**
 * @file grid.c
 * @brief Grid interface implementation
 */

#include "sylves/grid.h"
#include "sylves/vector.h"
#include "sylves/mesh.h"
#include "grid_internal.h"
#include "grid_defaults.h"
#include "square_grid_internal.h"
#include "hex_grid_internal.h"
#include <stdlib.h>

/* Grid destruction */
void sylves_grid_destroy(SylvesGrid* grid) {
    if (grid && grid->vtable && grid->vtable->destroy) {
        grid->vtable->destroy(grid);
    }
}

/* Grid properties */
SylvesGridType sylves_grid_get_type(const SylvesGrid* grid) {
    if (!grid) return SYLVES_GRID_TYPE_CUSTOM;
    return grid->type;
}

bool sylves_grid_is_2d(const SylvesGrid* grid) {
    return sylves_grid_default_is_2d(grid);
}

bool sylves_grid_is_3d(const SylvesGrid* grid) {
    return sylves_grid_default_is_3d(grid);
}

bool sylves_grid_is_planar(const SylvesGrid* grid) {
    return sylves_grid_default_is_planar(grid);
}

bool sylves_grid_is_repeating(const SylvesGrid* grid) {
    return sylves_grid_default_is_repeating(grid);
}

bool sylves_grid_is_orientable(const SylvesGrid* grid) {
    return sylves_grid_default_is_orientable(grid);
}

bool sylves_grid_is_finite(const SylvesGrid* grid) {
    return sylves_grid_default_is_finite(grid);
}

int sylves_grid_get_coordinate_dimension(const SylvesGrid* grid) {
    return sylves_grid_default_coordinate_dimension(grid);
}

/* Cell operations */
bool sylves_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    if (!grid || !grid->vtable || !grid->vtable->is_cell_in_grid) return false;
    return grid->vtable->is_cell_in_grid(grid, cell);
}

const SylvesCellType* sylves_grid_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_type) return NULL;
    return grid->vtable->get_cell_type(grid, cell);
}

/* Topology */
bool sylves_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                          SylvesCell* dest, SylvesCellDir* inverse_dir, 
                          SylvesConnection* connection) {
    if (!grid || !grid->vtable || !grid->vtable->try_move) return false;
    return grid->vtable->try_move(grid, cell, dir, dest, inverse_dir, connection);
}

int sylves_grid_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                              SylvesCellDir* dirs, size_t max_dirs) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_dirs) {
        return SYLVES_ERROR_NULL_POINTER;
    }
    return grid->vtable->get_cell_dirs(grid, cell, dirs, max_dirs);
}

int sylves_grid_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellCorner* corners, size_t max_corners) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_corners) {
        return SYLVES_ERROR_NULL_POINTER;
    }
    return grid->vtable->get_cell_corners(grid, cell, corners, max_corners);
}

/* Position and shape */
SylvesVector3 sylves_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_center) {
        return sylves_vector3_zero();
    }
    return grid->vtable->get_cell_center(grid, cell);
}

SylvesVector3 sylves_grid_get_cell_corner(const SylvesGrid* grid, SylvesCell cell,
                                          SylvesCellCorner corner) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_corner_pos) {
        return sylves_vector3_zero();
    }
    return grid->vtable->get_cell_corner_pos(grid, cell, corner);
}

int sylves_grid_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                            SylvesVector3* vertices, size_t max_vertices) {
    if (!grid || !grid->vtable || !grid->vtable->get_polygon) {
        return SYLVES_ERROR_NULL_POINTER;
    }
    return grid->vtable->get_polygon(grid, cell, vertices, max_vertices);
}

/* Queries */
bool sylves_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position,
                           SylvesCell* cell) {
    if (!grid || !grid->vtable || !grid->vtable->find_cell) return false;
    return grid->vtable->find_cell(grid, position, cell);
}

/* Implement remaining stub functions */
int sylves_grid_get_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells) {
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_square_grid_enumerate_cells(grid, cells, max_cells);
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_grid_enumerate_cells(grid, cells, max_cells);
        default:
            return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
}

int sylves_grid_get_cell_count(const SylvesGrid* grid) {
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_square_grid_cell_count(grid);
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_grid_cell_count(grid);
        default:
            return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
}

int sylves_grid_find_basic_path(const SylvesGrid* grid, SylvesCell start, SylvesCell dest,
                                SylvesCell* path, SylvesCellDir* dirs, size_t max_steps) {
    if (!grid || !GRID_VTABLE(grid)) return SYLVES_ERROR_NULL_POINTER;
    if (!sylves_grid_is_cell_in_grid(grid, start) || !sylves_grid_is_cell_in_grid(grid, dest)) {
        return SYLVES_ERROR_CELL_NOT_IN_GRID;
    }
    if (start.x == dest.x && start.y == dest.y && start.z == dest.z) {
        if (max_steps == 0) return 0;
        if (path) path[0] = start;
        return 1;
    }
    if (!sylves_grid_is_finite(grid)) {
        return SYLVES_ERROR_INFINITE_GRID;
    }
    int total = sylves_grid_get_cell_count(grid);
    if (total <= 0) return total; /* propagate error */

    typedef struct { SylvesCell cell; int parent; SylvesCellDir via_dir; } Node;
    Node* nodes = (Node*)calloc((size_t)total, sizeof(Node));
    if (!nodes) return SYLVES_ERROR_OUT_OF_MEMORY;

    int* visited = (int*)calloc((size_t)total, sizeof(int));
    if (!visited) { free(nodes); return SYLVES_ERROR_OUT_OF_MEMORY; }

    int head = 0, tail = 0;
    nodes[tail].cell = start; nodes[tail].parent = -1; nodes[tail].via_dir = 0; tail++;

    int found_index = -1;
    while (head < tail) {
        Node cur = nodes[head];
        if (cur.cell.x == dest.x && cur.cell.y == dest.y && cur.cell.z == dest.z) {
            found_index = head;
            break;
        }
        /* Expand neighbors */
        SylvesCellDir local_dirs[32];
        int dir_count = sylves_grid_get_cell_dirs(grid, cur.cell, local_dirs, 32);
        if (dir_count < 0) { free(visited); free(nodes); return dir_count; }
        for (int i = 0; i < dir_count; i++) {
            SylvesCell next_cell; SylvesCellDir inv; SylvesConnection conn;
            if (sylves_grid_try_move(grid, cur.cell, local_dirs[i], &next_cell, &inv, &conn)) {
                int seen = 0;
                for (int j = 0; j < tail; j++) {
                    if (nodes[j].cell.x == next_cell.x && nodes[j].cell.y == next_cell.y && nodes[j].cell.z == next_cell.z) { seen = 1; break; }
                }
                if (!seen) {
                    if (tail >= total) break; /* avoid overflow */
                    nodes[tail].cell = next_cell;
                    nodes[tail].parent = head;
                    nodes[tail].via_dir = local_dirs[i];
                    tail++;
                }
            }
        }
        head++;
        /* Optional guard to avoid runaway even on large finite grids */
        if ((size_t)head > max_steps * 64 && max_steps > 0) {
            break;
        }
    }

    int result = 0;
    if (found_index == -1) {
        result = SYLVES_ERROR_PATH_NOT_FOUND;
    } else {
        /* Reconstruct path */
        int chain_len = 0; int idx = found_index;
        while (idx != -1 && (size_t)chain_len < max_steps + 1) { chain_len++; idx = nodes[idx].parent; }
        if ((size_t)chain_len > max_steps && max_steps > 0) {
            result = SYLVES_ERROR_BUFFER_TOO_SMALL;
        } else {
            if (path) {
                int k = chain_len - 1; idx = found_index;
                while (idx != -1) {
                    path[k] = nodes[idx].cell;
                    k--; idx = nodes[idx].parent;
                }
            }
            if (dirs && chain_len > 1) {
                /* Fill directions between successive cells */
                int k = chain_len - 2; idx = found_index;
                while (nodes[idx].parent != -1) {
                    dirs[k] = nodes[idx].via_dir;
                    k--; idx = nodes[idx].parent;
                }
            }
            result = chain_len;
        }
    }

    free(visited);
    free(nodes);
    return result;
}

SylvesError sylves_grid_get_trs(const SylvesGrid* grid, SylvesCell cell, SylvesTRS* trs) {
    /* TODO: Implement */
    (void)grid; (void)cell; (void)trs;
    return SYLVES_ERROR_NOT_IMPLEMENTED;
}

SylvesError sylves_grid_get_mesh_data(const SylvesGrid* grid, SylvesCell cell,
                                      SylvesMeshData** mesh_data) {
    /* TODO: Implement */
    (void)grid; (void)cell; (void)mesh_data;
    return SYLVES_ERROR_NOT_IMPLEMENTED;
}

void sylves_mesh_data_free(SylvesMeshData* mesh_data) {
    if (mesh_data) {
        /* Use the proper mesh data destroyer */
        sylves_mesh_data_destroy(mesh_data);
    }
}

SylvesError sylves_grid_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell,
                                      SylvesAabb* aabb) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_aabb) {
        return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
    return grid->vtable->get_cell_aabb(grid, cell, aabb);
}

bool sylves_grid_find_cell_from_matrix(const SylvesGrid* grid, const SylvesMatrix4x4* matrix,
                                       SylvesCell* cell, SylvesCellRotation* rotation) {
    /* TODO: Implement */
    (void)grid; (void)matrix; (void)cell; (void)rotation;
    return false;
}

int sylves_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                  SylvesCell* cells, size_t max_cells) {
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_square_grid_get_cells_in_aabb(grid, min, max, cells, max_cells);
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_grid_get_cells_in_aabb(grid, min, max, cells, max_cells);
        default:
            return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
}

int sylves_grid_raycast(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction,
                       double max_distance, SylvesRaycastInfo* hits, size_t max_hits) {
    if (!grid || !GRID_VTABLE(grid) || !GRID_VTABLE(grid)->raycast) {
        return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
    return GRID_VTABLE(grid)->raycast(grid, origin, direction, max_distance, hits, max_hits);
}

const SylvesBound* sylves_grid_get_bound(const SylvesGrid* grid) {
    if (!grid) return NULL;
    return grid->bound;
}

SylvesGrid* sylves_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound) {
    if (!grid || !bound) return NULL;
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_square_grid_bound_by(grid, bound);
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_grid_bound_by(grid, bound);
        default:
            return NULL;
    }
}

SylvesGrid* sylves_grid_unbounded(const SylvesGrid* grid) {
    if (!grid) return NULL;
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_square_grid_unbounded_clone(grid);
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_grid_unbounded_clone(grid);
        default:
            return NULL;
    }
}

SylvesGrid* sylves_grid_get_dual(const SylvesGrid* grid) {
    /* TODO: Implement */
    (void)grid;
    return NULL;
}

SylvesGrid* sylves_grid_get_diagonal(const SylvesGrid* grid) {
    /* TODO: Implement */
    (void)grid;
    return NULL;
}

int sylves_grid_get_index_count(const SylvesGrid* grid) {
    if (!grid || !grid->vtable || !grid->vtable->get_index_count) {
        return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
    return grid->vtable->get_index_count(grid);
}

int sylves_grid_get_index(const SylvesGrid* grid, SylvesCell cell) {
    if (!grid || !grid->vtable || !grid->vtable->get_index) {
        return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
    return grid->vtable->get_index(grid, cell);
}

SylvesError sylves_grid_get_cell_by_index(const SylvesGrid* grid, int index, SylvesCell* cell) {
    if (!grid || !grid->vtable || !grid->vtable->get_cell_by_index) {
        return SYLVES_ERROR_NOT_IMPLEMENTED;
    }
    return grid->vtable->get_cell_by_index(grid, index, cell);
}
