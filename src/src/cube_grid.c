/**
 * @file cube_grid.c
 * @brief Implementation of 3D cubic grid
 */

#include "sylves/cube_grid.h"
#include "sylves/grid.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "sylves/aabb.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "sylves/errors.h"
#include "internal/grid_internal.h"
#include "sylves/cube_cell_type.h"
#include "sylves/utils.h"
#include "sylves/mesh.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    SylvesGrid base;
    double cell_size_x;
    double cell_size_y;
    double cell_size_z;
    bool is_bounded;
    int min_x, min_y, min_z;
    int max_x, max_y, max_z;
} CubeGrid;

/* Forward declarations */
static void cube_grid_destroy(SylvesGrid* grid);
static bool cube_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static bool cube_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                               SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static SylvesVector3 cube_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static SylvesVector3 cube_grid_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner);
static SylvesError cube_grid_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb);
static bool cube_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

/* VTable */
static const SylvesGridVTable cube_grid_vtable = {
    .destroy = cube_grid_destroy,
    .is_2d = NULL,
    .is_3d = NULL,
    .is_planar = NULL,
    .is_repeating = NULL,
    .is_orientable = NULL,
    .is_finite = NULL,
    .get_coordinate_dimension = NULL,
    .is_cell_in_grid = cube_grid_is_cell_in_grid,
    .get_cell_type = NULL,
    .try_move = cube_grid_try_move,
    .get_cell_dirs = NULL,
    .get_cell_corners = NULL,
    .get_cell_center = cube_grid_get_cell_center,
    .get_cell_corner_pos = cube_grid_get_cell_corner_pos,
    .get_polygon = NULL, /* 3D grid, no 2D polygon */
    .get_cell_aabb = cube_grid_get_cell_aabb,
    .find_cell = cube_grid_find_cell,
    .raycast = NULL,
    .get_index_count = NULL,
    .get_index = NULL,
    .get_cell_by_index = NULL
};

/* Helper functions */
static void cube_grid_destroy(SylvesGrid* grid) {
    if (grid) {
        sylves_free(grid->data);
        sylves_free(grid);
    }
}

static bool cube_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    const CubeGrid* cg = (const CubeGrid*)grid->data;
    
    if (!cg->is_bounded) {
        return true;
    }
    
    return cell.x >= cg->min_x && cell.x <= cg->max_x &&
           cell.y >= cg->min_y && cell.y <= cg->max_y &&
           cell.z >= cg->min_z && cell.z <= cg->max_z;
}

static bool cube_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                               SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    
    if (!cube_grid_is_cell_in_grid(grid, cell)) {
        return false;
    }
    
    if (dir < 0 || dir >= SYLVES_CUBE_DIR_COUNT) {
        return false;
    }
    
    /* Calculate destination */
    SylvesCell next = cell;
    SylvesCellDir inv_dir;
    
    switch (dir) {
        case SYLVES_CUBE_DIR_RIGHT:
            next.x++;
            inv_dir = SYLVES_CUBE_DIR_LEFT;
            break;
        case SYLVES_CUBE_DIR_LEFT:
            next.x--;
            inv_dir = SYLVES_CUBE_DIR_RIGHT;
            break;
        case SYLVES_CUBE_DIR_UP:
            next.y++;
            inv_dir = SYLVES_CUBE_DIR_DOWN;
            break;
        case SYLVES_CUBE_DIR_DOWN:
            next.y--;
            inv_dir = SYLVES_CUBE_DIR_UP;
            break;
        case SYLVES_CUBE_DIR_FORWARD:
            next.z++;
            inv_dir = SYLVES_CUBE_DIR_BACK;
            break;
        case SYLVES_CUBE_DIR_BACK:
            next.z--;
            inv_dir = SYLVES_CUBE_DIR_FORWARD;
            break;
        default:
            return false;
    }
    
    if (!cube_grid_is_cell_in_grid(grid, next)) {
        return false;
    }
    
    if (dest) *dest = next;
    if (inverse_dir) *inverse_dir = inv_dir;
    if (connection) {
        connection->rotation = 0; /* Identity rotation */
        connection->is_mirror = false;
    }
    
    return true;
}

static SylvesVector3 cube_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    const CubeGrid* cg = (const CubeGrid*)grid->data;
    
    return (SylvesVector3){
        .x = (cell.x + 0.5) * cg->cell_size_x,
        .y = (cell.y + 0.5) * cg->cell_size_y,
        .z = (cell.z + 0.5) * cg->cell_size_z
    };
}

static SylvesVector3 cube_grid_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner) {
    const CubeGrid* cg = (const CubeGrid*)grid->data;
    
    if (corner < 0 || corner >= SYLVES_CUBE_CORNER_COUNT) {
        return (SylvesVector3){0, 0, 0};
    }
    
    /* Corner positions based on naming convention:
     * BACK_DOWN_LEFT = 0, BACK_DOWN_RIGHT = 1, etc.
     */
    double x_offset = (corner & 1) ? 1.0 : 0.0;      /* RIGHT bit */
    double y_offset = (corner & 2) ? 1.0 : 0.0;      /* UP bit */
    double z_offset = (corner & 4) ? 1.0 : 0.0;      /* FORWARD bit */
    
    return (SylvesVector3){
        .x = (cell.x + x_offset) * cg->cell_size_x,
        .y = (cell.y + y_offset) * cg->cell_size_y,
        .z = (cell.z + z_offset) * cg->cell_size_z
    };
}

static SylvesError cube_grid_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb) {
    const CubeGrid* cg = (const CubeGrid*)grid->data;
    
    if (!cube_grid_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_INVALID_CELL;
    }
    
    aabb->min = (SylvesVector3){
        .x = cell.x * cg->cell_size_x,
        .y = cell.y * cg->cell_size_y,
        .z = cell.z * cg->cell_size_z
    };
    
    aabb->max = (SylvesVector3){
        .x = (cell.x + 1) * cg->cell_size_x,
        .y = (cell.y + 1) * cg->cell_size_y,
        .z = (cell.z + 1) * cg->cell_size_z
    };
    
    return SYLVES_SUCCESS;
}

static bool cube_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    const CubeGrid* cg = (const CubeGrid*)grid->data;
    
    cell->x = (int)floor(position.x / cg->cell_size_x);
    cell->y = (int)floor(position.y / cg->cell_size_y);
    cell->z = (int)floor(position.z / cg->cell_size_z);
    
    if (!cube_grid_is_cell_in_grid(grid, *cell)) {
        return false;
    }
    
    return true;
}

/* Creation functions */
static SylvesGrid* create_cube_grid_internal(double cell_size_x, double cell_size_y, double cell_size_z,
                                             bool is_bounded, int min_x, int min_y, int min_z,
                                             int max_x, int max_y, int max_z) {
    CubeGrid* grid = sylves_alloc(sizeof(CubeGrid));
    if (!grid) {
        return NULL;
    }
    
    /* Initialize base grid */
    grid->base.vtable = &cube_grid_vtable;
    grid->base.type = SYLVES_GRID_TYPE_CUBE;
    grid->base.bound = NULL;
    grid->base.data = grid;
    
    /* Initialize cube-specific data */
    grid->cell_size_x = cell_size_x;
    grid->cell_size_y = cell_size_y;
    grid->cell_size_z = cell_size_z;
    grid->is_bounded = is_bounded;
    grid->min_x = min_x;
    grid->min_y = min_y;
    grid->min_z = min_z;
    grid->max_x = max_x;
    grid->max_y = max_y;
    grid->max_z = max_z;
    
    return &grid->base;
}

SylvesGrid* sylves_cube_grid_create(double cell_size) {
    if (cell_size <= 0) {
        return NULL;
    }
    return create_cube_grid_internal(cell_size, cell_size, cell_size, false, 0, 0, 0, 0, 0, 0);
}

SylvesGrid* sylves_cube_grid_create_anisotropic(double cell_size_x, double cell_size_y, double cell_size_z) {
    if (cell_size_x <= 0 || cell_size_y <= 0 || cell_size_z <= 0) {
        return NULL;
    }
    return create_cube_grid_internal(cell_size_x, cell_size_y, cell_size_z, false, 0, 0, 0, 0, 0, 0);
}

SylvesGrid* sylves_cube_grid_create_bounded(double cell_size, 
                                            int min_x, int min_y, int min_z,
                                            int max_x, int max_y, int max_z) {
    if (cell_size <= 0 || min_x > max_x || min_y > max_y || min_z > max_z) {
        return NULL;
    }
    return create_cube_grid_internal(cell_size, cell_size, cell_size, true, min_x, min_y, min_z, max_x, max_y, max_z);
}

SylvesGrid* sylves_cube_grid_create_bounded_anisotropic(double cell_size_x, double cell_size_y, double cell_size_z,
                                                        int min_x, int min_y, int min_z,
                                                        int max_x, int max_y, int max_z) {
    if (cell_size_x <= 0 || cell_size_y <= 0 || cell_size_z <= 0 || 
        min_x > max_x || min_y > max_y || min_z > max_z) {
        return NULL;
    }
    return create_cube_grid_internal(cell_size_x, cell_size_y, cell_size_z, true, min_x, min_y, min_z, max_x, max_y, max_z);
}
