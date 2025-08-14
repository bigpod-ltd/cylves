/**
 * @file square_grid.c
 * @brief Square grid implementation
 */

#include "sylves/square_grid.h"
#include "sylves/vector.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "grid_internal.h"
#include "square_grid_internal.h"
#include "sylves/bounds.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

/* Square grid specific data */
typedef struct {
    double cell_size;
    int min_x, min_y;
    int max_x, max_y;
    bool is_bounded;
} SquareGridData;

/* Forward declarations */
static const SylvesCellType* square_get_cell_type(const SylvesGrid* grid, SylvesCell cell);
static void square_destroy(SylvesGrid* grid);
static bool square_is_2d(const SylvesGrid* grid);
static bool square_is_3d(const SylvesGrid* grid);
static bool square_is_planar(const SylvesGrid* grid);
static bool square_is_repeating(const SylvesGrid* grid);
static bool square_is_orientable(const SylvesGrid* grid);
static bool square_is_finite(const SylvesGrid* grid);
static int square_get_coordinate_dimension(const SylvesGrid* grid);
static bool square_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static bool square_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                           SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static int square_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                               SylvesCellDir* dirs, size_t max_dirs);
static int square_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                  SylvesCellCorner* corners, size_t max_corners);
static SylvesVector3 square_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static SylvesVector3 square_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                               SylvesCellCorner corner);
static int square_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                             SylvesVector3* vertices, size_t max_vertices);
static bool square_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

/* Forward declarations of indexing helpers used in vtable */
static int square_get_index_count(const SylvesGrid* grid);
static int square_get_index(const SylvesGrid* grid, SylvesCell cell);
static SylvesError square_get_cell_by_index(const SylvesGrid* grid, int index, SylvesCell* cell);

/* VTable for square grid */
static const SylvesGridVTable square_vtable = {
    .destroy = square_destroy,
    .is_2d = square_is_2d,
    .is_3d = square_is_3d,
    .is_planar = square_is_planar,
    .is_repeating = square_is_repeating,
    .is_orientable = square_is_orientable,
    .is_finite = square_is_finite,
    .get_coordinate_dimension = square_get_coordinate_dimension,
    .is_cell_in_grid = square_is_cell_in_grid,
    .get_cell_type = square_get_cell_type,
    .try_move = square_try_move,
    .get_cell_dirs = square_get_cell_dirs,
    .get_cell_corners = square_get_cell_corners,
    .get_cell_center = square_get_cell_center,
    .get_cell_corner_pos = square_get_cell_corner_pos,
    .get_polygon = square_get_polygon,
    .get_cell_aabb = NULL, /* TODO: implement */
    .find_cell = square_find_cell,
    .get_index_count = square_get_index_count,
    .get_index = square_get_index,
    .get_cell_by_index = square_get_cell_by_index
};

/* Public API */

SylvesGrid* sylves_square_grid_create(double cell_size) {
    if (cell_size <= 0.0) {
        return NULL;
    }
    
    SylvesGrid* grid = (SylvesGrid*)calloc(1, sizeof(SylvesGrid));
    if (!grid) {
        return NULL;
    }
    
    SquareGridData* data = (SquareGridData*)calloc(1, sizeof(SquareGridData));
    if (!data) {
        free(grid);
        return NULL;
    }
    
    data->cell_size = cell_size;
    data->is_bounded = false;
    
    grid->vtable = &square_vtable;
    grid->type = SYLVES_GRID_TYPE_SQUARE;
    grid->bound = NULL;
    grid->data = data;
    
    return grid;
}

SylvesGrid* sylves_square_grid_create_bounded(double cell_size, 
                                              int min_x, int min_y,
                                              int max_x, int max_y) {
    SylvesGrid* grid = sylves_square_grid_create(cell_size);
    if (!grid) {
        return NULL;
    }
    
    SquareGridData* data = (SquareGridData*)grid->data;
    data->min_x = min_x;
    data->min_y = min_y;
    data->max_x = max_x;
    data->max_y = max_y;
    data->is_bounded = true;

    /* attach a bound object */
    grid->bound = sylves_bound_create_rectangle(min_x, min_y, max_x, max_y);
    
    return grid;
}

/* Implementation of vtable functions */

static void square_destroy(SylvesGrid* grid) {
    if (grid) {
        if (grid->bound) {
            sylves_bound_destroy((SylvesBound*)grid->bound);
        }
        free(grid->data);
        free(grid);
    }
}

static bool square_is_2d(const SylvesGrid* grid) {
    return true;
}

static bool square_is_3d(const SylvesGrid* grid) {
    return false;
}

static bool square_is_planar(const SylvesGrid* grid) {
    return true;
}

static bool square_is_repeating(const SylvesGrid* grid) {
    return true;
}

static bool square_is_orientable(const SylvesGrid* grid) {
    return true;
}

static bool square_is_finite(const SylvesGrid* grid) {
    SquareGridData* data = (SquareGridData*)grid->data;
    return data->is_bounded;
}

static int square_get_coordinate_dimension(const SylvesGrid* grid) {
    return 2;
}

static const SylvesCellType* square_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    (void)cell;
    /* Singleton square cell type */
    static SylvesCellType* square_ct = NULL;
    if (!square_ct) {
        square_ct = sylves_square_cell_type_create();
    }
    return square_ct;
}

static bool square_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    if (cell.z != 0) {
        return false;
    }
    
    SquareGridData* data = (SquareGridData*)grid->data;
    if (!data->is_bounded) {
        return true;
    }
    
    return cell.x >= data->min_x && cell.x <= data->max_x &&
           cell.y >= data->min_y && cell.y <= data->max_y;
}

static bool square_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                           SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    if (!square_is_cell_in_grid(grid, cell)) {
        return false;
    }
    
    SylvesCell new_cell = cell;
    SylvesCellDir inv_dir;
    
    switch (dir) {
        case SYLVES_SQUARE_DIR_RIGHT:
            new_cell.x++;
            inv_dir = SYLVES_SQUARE_DIR_LEFT;
            break;
        case SYLVES_SQUARE_DIR_UP:
            new_cell.y++;
            inv_dir = SYLVES_SQUARE_DIR_DOWN;
            break;
        case SYLVES_SQUARE_DIR_LEFT:
            new_cell.x--;
            inv_dir = SYLVES_SQUARE_DIR_RIGHT;
            break;
        case SYLVES_SQUARE_DIR_DOWN:
            new_cell.y--;
            inv_dir = SYLVES_SQUARE_DIR_UP;
            break;
        default:
            return false;
    }
    
    if (!square_is_cell_in_grid(grid, new_cell)) {
        return false;
    }
    
    if (dest) *dest = new_cell;
    if (inverse_dir) *inverse_dir = inv_dir;
    if (connection) {
        connection->rotation = 0; /* Identity rotation */
        connection->is_mirror = false;
    }
    
    return true;
}

static int square_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                               SylvesCellDir* dirs, size_t max_dirs) {
    if (!square_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_CELL_NOT_IN_GRID;
    }
    
    int count = 0;
    for (int i = 0; i < SYLVES_SQUARE_DIR_COUNT && count < max_dirs; i++) {
        SylvesCell dest;
        if (square_try_move(grid, cell, i, &dest, NULL, NULL)) {
            if (dirs) dirs[count] = i;
            count++;
        }
    }
    
    return count;
}

static int square_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                  SylvesCellCorner* corners, size_t max_corners) {
    if (!square_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_CELL_NOT_IN_GRID;
    }
    
    int count = (max_corners < SYLVES_SQUARE_CORNER_COUNT) ? max_corners : SYLVES_SQUARE_CORNER_COUNT;
    
    if (corners) {
        for (int i = 0; i < count; i++) {
            corners[i] = i;
        }
    }
    
    return count;
}

static SylvesVector3 square_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    SquareGridData* data = (SquareGridData*)grid->data;
    return sylves_vector3_create(
        (cell.x + 0.5) * data->cell_size,
        (cell.y + 0.5) * data->cell_size,
        0.0
    );
}

static SylvesVector3 square_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                               SylvesCellCorner corner) {
    SquareGridData* data = (SquareGridData*)grid->data;
    double x = cell.x * data->cell_size;
    double y = cell.y * data->cell_size;
    
    switch (corner) {
        case SYLVES_SQUARE_CORNER_BOTTOM_RIGHT:
            x += data->cell_size;
            break;
        case SYLVES_SQUARE_CORNER_TOP_RIGHT:
            x += data->cell_size;
            y += data->cell_size;
            break;
        case SYLVES_SQUARE_CORNER_TOP_LEFT:
            y += data->cell_size;
            break;
        case SYLVES_SQUARE_CORNER_BOTTOM_LEFT:
        default:
            break;
    }
    
    return sylves_vector3_create(x, y, 0.0);
}

static int square_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                             SylvesVector3* vertices, size_t max_vertices) {
    if (!square_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_CELL_NOT_IN_GRID;
    }
    
    if (max_vertices < 4) {
        return SYLVES_ERROR_BUFFER_TOO_SMALL;
    }
    
    if (vertices) {
        for (int i = 0; i < 4; i++) {
            vertices[i] = square_get_cell_corner_pos(grid, cell, i);
        }
    }
    
    return 4;
}

static bool square_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    SquareGridData* data = (SquareGridData*)grid->data;
    
    int x = (int)floor(position.x / data->cell_size);
    int y = (int)floor(position.y / data->cell_size);
    
    SylvesCell found = sylves_cell_create_2d(x, y);
    
    if (!square_is_cell_in_grid(grid, found)) {
        return false;
    }
    
    if (cell) *cell = found;
    return true;
}

/* Internal helpers for enumeration used by generic grid functions */
int sylves_square_grid_enumerate_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells) {
    const SquareGridData* data = (const SquareGridData*)grid->data;
    if (!data->is_bounded) {
        return SYLVES_ERROR_INFINITE_GRID;
    }
    long long width = (long long)data->max_x - (long long)data->min_x + 1;
    long long height = (long long)data->max_y - (long long)data->min_y + 1;
    long long total = width * height;
    if (total < 0) return SYLVES_ERROR_INVALID_STATE;

    size_t to_write = (size_t)total;
    if (cells) {
        size_t count = 0;
        for (int y = data->min_y; y <= data->max_y && count < max_cells; y++) {
            for (int x = data->min_x; x <= data->max_x && count < max_cells; x++) {
                cells[count++] = sylves_cell_create_2d(x, y);
            }
        }
        return (int)count;
    }
    return (int)((to_write < max_cells) ? to_write : max_cells);
}

/* Indexing helpers for bounded square grid */
static int square_get_index_count(const SylvesGrid* grid) {
    const SquareGridData* d = (const SquareGridData*)grid->data;
    if (!d->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    long long w = (long long)d->max_x - (long long)d->min_x + 1;
    long long h = (long long)d->max_y - (long long)d->min_y + 1;
    long long total = w * h;
    if (total < 0 || total > INT_MAX) return SYLVES_ERROR_INVALID_STATE;
    return (int)total;
}

static int square_get_index(const SylvesGrid* grid, SylvesCell cell) {
    const SquareGridData* d = (const SquareGridData*)grid->data;
    if (!d->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    if (!square_is_cell_in_grid(grid, cell)) return SYLVES_ERROR_CELL_NOT_IN_GRID;
    int w = d->max_x - d->min_x + 1;
    int ix = cell.x - d->min_x;
    int iy = cell.y - d->min_y;
    long long idx = (long long)iy * (long long)w + (long long)ix;
    if (idx < 0 || idx > INT_MAX) return SYLVES_ERROR_INVALID_STATE;
    return (int)idx;
}

static SylvesError square_get_cell_by_index(const SylvesGrid* grid, int index, SylvesCell* cell) {
    const SquareGridData* d = (const SquareGridData*)grid->data;
    if (!d->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    int count = square_get_index_count(grid);
    if (count < 0) return (SylvesError)count;
    if (index < 0 || index >= count) return SYLVES_ERROR_OUT_OF_BOUNDS;
    int w = d->max_x - d->min_x + 1;
    int iy = index / w;
    int ix = index % w;
    if (cell) *cell = (SylvesCell){ d->min_x + ix, d->min_y + iy, 0 };
    return SYLVES_SUCCESS;
}

int sylves_square_grid_cell_count(const SylvesGrid* grid) {
    const SquareGridData* data = (const SquareGridData*)grid->data;
    if (!data->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    long long width = (long long)data->max_x - (long long)data->min_x + 1;
    long long height = (long long)data->max_y - (long long)data->min_y + 1;
    long long total = width * height;
    if (total < 0 || total > INT_MAX) return SYLVES_ERROR_INVALID_STATE;
    return (int)total;
}

int sylves_square_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                         SylvesCell* cells, size_t max_cells) {
    const SquareGridData* data = (const SquareGridData*)grid->data;
    double s = data->cell_size;
    /* Compute inclusive integer cell range overlapping the AABB */
    int min_cx = (int)floor(min.x / s);
    int min_cy = (int)floor(min.y / s);
    /* Subtract tiny epsilon to avoid including the next cell when max lies on boundary */
    const double eps = 1e-9;
    int max_cx = (int)floor((max.x - eps) / s);
    int max_cy = (int)floor((max.y - eps) / s);
    if (data->is_bounded) {
        if (min_cx < data->min_x) min_cx = data->min_x;
        if (min_cy < data->min_y) min_cy = data->min_y;
        if (max_cx > data->max_x) max_cx = data->max_x;
        if (max_cy > data->max_y) max_cy = data->max_y;
    }
    if (max_cx < min_cx || max_cy < min_cy) return 0;

    size_t count = 0;
    for (int y = min_cy; y <= max_cy && count < max_cells; y++) {
        for (int x = min_cx; x <= max_cx && count < max_cells; x++) {
            if (!data->is_bounded || (x >= data->min_x && x <= data->max_x && y >= data->min_y && y <= data->max_y)) {
                if (cells) cells[count] = sylves_cell_create_2d(x, y);
                count++;
            }
        }
    }
    return (int)count;
}

SylvesGrid* sylves_square_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound) {
    if (!grid || !bound) return NULL;
    const SquareGridData* sd = (const SquareGridData*)grid->data;
    int bxmin, bymin, bxmax, bymax;
    if (sylves_bound_get_rect(bound, &bxmin, &bymin, &bxmax, &bymax) != 0) {
        return NULL; /* not a rectangle bound */
    }
    int min_x = bxmin, min_y = bymin, max_x = bxmax, max_y = bymax;
    if (sd->is_bounded) {
        if (min_x < sd->min_x) min_x = sd->min_x;
        if (min_y < sd->min_y) min_y = sd->min_y;
        if (max_x > sd->max_x) max_x = sd->max_x;
        if (max_y > sd->max_y) max_y = sd->max_y;
        if (max_x < min_x || max_y < min_y) return NULL;
    }
    return sylves_square_grid_create_bounded(sd->cell_size, min_x, min_y, max_x, max_y);
}

SylvesGrid* sylves_square_grid_unbounded_clone(const SylvesGrid* grid) {
    if (!grid) return NULL;
    const SquareGridData* sd = (const SquareGridData*)grid->data;
    return sylves_square_grid_create(sd->cell_size);
}
