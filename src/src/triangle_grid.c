/**
 * @file triangle_grid.c
 * @brief Triangle grid implementation
 */

#include "sylves/triangle_grid.h"
#include "sylves/vector.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "sylves/bounds.h"
#include "grid_internal.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Triangle grid specific data */
typedef struct {
    double cell_size;
    SylvesTriangleOrientation orientation;
    bool is_bounded;
    SylvesVector3Int min;
    SylvesVector3Int max;
} TriangleGridData;

/* Forward declarations */
static const SylvesCellType* triangle_get_cell_type(const SylvesGrid* grid, SylvesCell cell);
static void triangle_destroy(SylvesGrid* grid);
static bool triangle_is_2d(const SylvesGrid* grid);
static bool triangle_is_3d(const SylvesGrid* grid);
static bool triangle_is_planar(const SylvesGrid* grid);
static bool triangle_is_repeating(const SylvesGrid* grid);
static bool triangle_is_orientable(const SylvesGrid* grid);
static bool triangle_is_finite(const SylvesGrid* grid);
static int triangle_get_coordinate_dimension(const SylvesGrid* grid);
static bool triangle_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static bool triangle_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir, SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static SylvesVector3 triangle_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static int triangle_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellDir* dirs, size_t max_dirs);
static int triangle_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                    SylvesCellCorner* corners, size_t max_corners);
static int triangle_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                               SylvesVector3* vertices, size_t max_vertices);
static bool triangle_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

/* VTable for triangle grid */
static const SylvesGridVTable triangle_vtable = {
    .destroy = triangle_destroy,
    .is_2d = triangle_is_2d,
    .is_3d = triangle_is_3d,
    .is_planar = triangle_is_planar,
    .is_repeating = triangle_is_repeating,
    .is_orientable = triangle_is_orientable,
    .is_finite = triangle_is_finite,
    .get_coordinate_dimension = triangle_get_coordinate_dimension,
    .is_cell_in_grid = triangle_is_cell_in_grid,
    .get_cell_type = triangle_get_cell_type,
    .try_move = triangle_try_move,
    .get_cell_dirs = triangle_get_cell_dirs,
    .get_cell_corners = triangle_get_cell_corners,
    .get_cell_center = triangle_get_cell_center,
    .get_polygon = triangle_get_polygon,
    .find_cell = triangle_find_cell,
};

/* Public API */

static void triangle_destroy(SylvesGrid* grid) {
    if (grid) {
        free(grid->data);
        free(grid);
    }
}

static bool triangle_is_2d(const SylvesGrid* grid) {
    return true;
}

static bool triangle_is_3d(const SylvesGrid* grid) {
    return false;
}

static bool triangle_is_planar(const SylvesGrid* grid) {
    return true;
}

static bool triangle_is_repeating(const SylvesGrid* grid) {
    return true;
}

static bool triangle_is_orientable(const SylvesGrid* grid) {
    return true;
}

static bool triangle_is_finite(const SylvesGrid* grid) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return data->is_bounded;
}

static int triangle_get_coordinate_dimension(const SylvesGrid* grid) {
    return 2;
}

static const SylvesCellType* triangle_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    (void)cell;
    /* Singleton triangle cell type */
    static SylvesCellType* triangle_ct = NULL;
    if (!triangle_ct) {
        /* triangle_ct = sylves_triangle_cell_type_create(); */
    }
    return triangle_ct;
}

static bool triangle_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    if (!data->is_bounded) {
        return true;
    }
    
    return cell.x >= data->min.x && cell.x <= data->max.x &&
           cell.y >= data->min.y && cell.y <= data->max.y &&
           cell.z >= data->min.z && cell.z <= data->max.z;
}

static SylvesVector3 triangle_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    double side = data->cell_size;
    if(data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED) {
        return (SylvesVector3){
            (0.5 * cell.x - 0.5 * cell.z) * side,
            (-1 / 3.0 * cell.x + 2 / 3.0 * cell.y - 1 / 3.0 * cell.z) * side,
            0};
    } else {
        return (SylvesVector3){
            (-1 / 3.0 * cell.y + 2 / 3.0 * cell.x - 1 / 3.0 * cell.z) * side,
            (0.5 * cell.y - 0.5 * cell.z) * side,
            0};
    }
}

static bool triangle_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                             SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    if (!triangle_is_cell_in_grid(grid, cell)) {
        return false;
    }
    
    TriangleGridData* data = (TriangleGridData*)grid->data;
    *inverse_dir = (3 + dir) % 6;
    
    if (data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED) {
        switch (dir) {
            case 0: /* UpRight */
                *dest = (SylvesCell){cell.x, cell.y, cell.z - 1}; 
                break;
            case 1: /* Up */
                *dest = (SylvesCell){cell.x, cell.y + 1, cell.z}; 
                break;
            case 2: /* UpLeft */
                *dest = (SylvesCell){cell.x - 1, cell.y, cell.z}; 
                break;
            case 3: /* DownLeft */
                *dest = (SylvesCell){cell.x, cell.y, cell.z + 1}; 
                break;
            case 4: /* Down */
                *dest = (SylvesCell){cell.x, cell.y - 1, cell.z}; 
                break;
            case 5: /* DownRight */
                *dest = (SylvesCell){cell.x + 1, cell.y, cell.z}; 
                break;
            default:
                return false;
        }
    } else {
        switch (dir) {
            case 0: /* Right */
                *dest = (SylvesCell){cell.x + 1, cell.y, cell.z}; 
                break;
            case 1: /* UpRight */
                *dest = (SylvesCell){cell.x, cell.y, cell.z - 1}; 
                break;
            case 2: /* UpLeft */
                *dest = (SylvesCell){cell.x, cell.y + 1, cell.z}; 
                break;
            case 3: /* Left */
                *dest = (SylvesCell){cell.x - 1, cell.y, cell.z}; 
                break;
            case 4: /* DownLeft */
                *dest = (SylvesCell){cell.x, cell.y, cell.z + 1}; 
                break;
            case 5: /* DownRight */
                *dest = (SylvesCell){cell.x, cell.y - 1, cell.z}; 
                break;
            default:
                return false;
        }
    }
    
    connection->rotation = 0;
    connection->is_mirror = false;
    
    return triangle_is_cell_in_grid(grid, *dest);
}

static bool triangle_is_up(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED && 
           cell.x + cell.y + cell.z == 2;
}

static bool triangle_is_down(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED && 
           cell.x + cell.y + cell.z == 1;
}

static bool triangle_is_left(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_SIDES && 
           cell.x + cell.y + cell.z == 1;
}

static bool triangle_is_right(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_SIDES && 
           cell.x + cell.y + cell.z == 2;
}

static bool triangle_is_up_or_right(const SylvesGrid* grid, SylvesCell cell) {
    return cell.x + cell.y + cell.z == 2;
}

static bool triangle_is_up_or_left(const SylvesGrid* grid, SylvesCell cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    return (data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_SIDES) ^ 
           (cell.x + cell.y + cell.z == 2);
}

static int triangle_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellDir* dirs, size_t max_dirs) {
    if (max_dirs < 3) return -1;
    
    /* Triangle has alternating orientation - some have 3 valid dirs, some have different 3 */
    if (triangle_is_up_or_left(grid, cell)) {
        /* Directions: UpRight(0), UpLeft(2), DownLeft(3) for FlatTopped */
        /* Or: Right(0), UpLeft(2), DownLeft(4) for FlatSides */
        dirs[0] = 0;
        dirs[1] = 2; 
        dirs[2] = triangle_is_up(grid, cell) ? 3 : 4;
    } else {
        /* Directions: Up(1), Down(4), DownRight(5) for FlatTopped */
        /* Or: UpRight(1), Left(3), DownRight(5) for FlatSides */
        dirs[0] = 1;
        dirs[1] = triangle_is_down(grid, cell) ? 4 : 3;
        dirs[2] = 5;
    }
    return 3;
}

static int triangle_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                    SylvesCellCorner* corners, size_t max_corners) {
    if (max_corners < 3) return -1;
    
    /* Triangle corners based on orientation */
    if (triangle_is_up_or_right(grid, cell)) {
        /* DownRight(0), Up(2), DownLeft(4) */
        corners[0] = 0;
        corners[1] = 2;
        corners[2] = 4;
    } else {
        /* UpRight(1), UpLeft(3), Down(5) */  
        corners[0] = 1;
        corners[1] = 3;
        corners[2] = 5;
    }
    return 3;
}

static int triangle_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                               SylvesVector3* vertices, size_t max_vertices) {
    if (max_vertices < 3) return -1;
    
    TriangleGridData* data = (TriangleGridData*)grid->data;
    SylvesVector3 center = triangle_get_cell_center(grid, cell);
    double scale = data->cell_size;
    
    /* The triangle polygons, scaled to fit based on orientation */
    if (data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED) {
        if (triangle_is_up(grid, cell)) {
            /* Up-pointing triangle */
            vertices[0] = (SylvesVector3){center.x + 0.5 * scale, center.y - scale/3.0, 0};
            vertices[1] = (SylvesVector3){center.x, center.y + 2.0*scale/3.0, 0};
            vertices[2] = (SylvesVector3){center.x - 0.5 * scale, center.y - scale/3.0, 0};
        } else {
            /* Down-pointing triangle */
            vertices[0] = (SylvesVector3){center.x + 0.5 * scale, center.y + scale/3.0, 0};
            vertices[1] = (SylvesVector3){center.x - 0.5 * scale, center.y + scale/3.0, 0};
            vertices[2] = (SylvesVector3){center.x, center.y - 2.0*scale/3.0, 0};
        }
    } else {
        /* FlatSides orientation */
        double sqrt3_2 = 0.86602540378444; /* sqrt(3)/2 */
        if (triangle_is_right(grid, cell)) {
            /* Right-pointing triangle */
            vertices[0] = (SylvesVector3){center.x + 2.0*scale/3.0, center.y, 0};
            vertices[1] = (SylvesVector3){center.x - scale/3.0, center.y + 0.5*scale, 0};
            vertices[2] = (SylvesVector3){center.x - scale/3.0, center.y - 0.5*scale, 0};
        } else {
            /* Left-pointing triangle */
            vertices[0] = (SylvesVector3){center.x + scale/3.0, center.y + 0.5*scale, 0};
            vertices[1] = (SylvesVector3){center.x - 2.0*scale/3.0, center.y, 0};
            vertices[2] = (SylvesVector3){center.x + scale/3.0, center.y - 0.5*scale, 0};
        }
    }
    
    return 3;
}

static bool triangle_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    TriangleGridData* data = (TriangleGridData*)grid->data;
    
    if (data->orientation == SYLVES_TRIANGLE_ORIENTATION_FLAT_SIDES) {
        double x = position.x / data->cell_size;
        double y = position.y / data->cell_size;
        *cell = (SylvesCell){
            (int)floor(x) + 1,
            (int)ceil(y - 0.5 * x),
            (int)ceil(-y - 0.5 * x)
        };
    } else {
        /* FlatTopped orientation */
        double x = position.x / data->cell_size;
        double y = position.y / data->cell_size;
        *cell = (SylvesCell){
            (int)ceil(x - 0.5 * y),
            (int)floor(y) + 1,
            (int)ceil(-x - 0.5 * y)
        };
    }
    
    return triangle_is_cell_in_grid(grid, *cell);
}

SylvesGrid* sylves_triangle_grid_create(double cell_size, SylvesTriangleOrientation orientation) {
    if (cell_size <= 0.0) {
        return NULL;
    }

    SylvesGrid* grid = (SylvesGrid*)calloc(1, sizeof(SylvesGrid));
    if (!grid) {
        return NULL;
    }

    TriangleGridData* data = (TriangleGridData*)calloc(1, sizeof(TriangleGridData));
    if (!data) {
        free(grid);
        return NULL;
    }

    data->cell_size = cell_size;
    data->orientation = orientation;
    data->is_bounded = false;

    grid->vtable = &triangle_vtable;
    grid->type = SYLVES_GRID_TYPE_TRIANGLE; /* Assuming type exists */
    grid->bound = NULL;
    grid->data = data;

    return grid;
}

SylvesGrid* sylves_triangle_grid_create_bounded(double cell_size, SylvesTriangleOrientation orientation,
                                                int min_x, int min_y, int min_z,
                                                int max_x, int max_y, int max_z) {
    if (cell_size <= 0.0) {
        return NULL;
    }

    SylvesGrid* grid = (SylvesGrid*)calloc(1, sizeof(SylvesGrid));
    if (!grid) {
        return NULL;
    }

    TriangleGridData* data = (TriangleGridData*)calloc(1, sizeof(TriangleGridData));
    if (!data) {
        free(grid);
        return NULL;
    }

    data->cell_size = cell_size;
    data->orientation = orientation;
    data->is_bounded = true;
    data->min = (SylvesVector3Int){min_x, min_y, min_z};
    data->max = (SylvesVector3Int){max_x, max_y, max_z};

    grid->vtable = &triangle_vtable;
    grid->type = SYLVES_GRID_TYPE_TRIANGLE;
    grid->bound = sylves_bound_create_triangle_parallelogram(min_x, min_y, min_z, max_x, max_y, max_z);
    grid->data = data;

    return grid;
}
