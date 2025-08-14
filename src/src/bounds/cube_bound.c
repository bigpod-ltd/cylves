/**
 * @file cube_bound.c
 * @brief Implementation of CubeBound - 3D rectangular bounds for cube grids
 */

#include "sylves/bounds.h"
#include "sylves/cell.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "../internal/bound_internal.h"
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

typedef struct {
    SylvesBound base;
    int min_x, min_y, min_z;
    int max_x, max_y, max_z;
} CubeBound;

/* Forward declarations */
static void cube_bound_destroy(SylvesBound* bound);
static bool cube_bound_contains(const SylvesBound* bound, SylvesCell cell);
static int cube_bound_get_cells(const SylvesBound* bound, SylvesCell* cells, size_t max_cells);
static const char* cube_bound_name(const SylvesBound* bound);
static int cube_bound_get_rect(const SylvesBound* bound, int* min_x, int* min_y, int* max_x, int* max_y);
static int cube_bound_get_cube(const SylvesBound* bound, int* min_x, int* min_y, int* min_z,
                               int* max_x, int* max_y, int* max_z);

/* VTable */
static const SylvesBoundVTable cube_bound_vtable = {
    .contains = cube_bound_contains,
    .destroy = cube_bound_destroy,
    .name = cube_bound_name,
    .get_cells = cube_bound_get_cells,
    .get_rect = cube_bound_get_rect,
    .get_cube = cube_bound_get_cube
};

/* Helper functions */
static void cube_bound_destroy(SylvesBound* bound) {
    if (bound) {
        sylves_free(bound);
    }
}

static bool cube_bound_contains(const SylvesBound* bound, SylvesCell cell) {
    const CubeBound* cb = (const CubeBound*)bound;
    return cell.x >= cb->min_x && cell.x <= cb->max_x &&
           cell.y >= cb->min_y && cell.y <= cb->max_y &&
           cell.z >= cb->min_z && cell.z <= cb->max_z;
}

static const char* cube_bound_name(const SylvesBound* bound) {
    (void)bound;
    return "CubeBound";
}

static int cube_bound_get_rect(const SylvesBound* bound, int* min_x, int* min_y, int* max_x, int* max_y) {
    const CubeBound* cb = (const CubeBound*)bound;
    if (min_x) *min_x = cb->min_x;
    if (min_y) *min_y = cb->min_y;
    if (max_x) *max_x = cb->max_x;
    if (max_y) *max_y = cb->max_y;
    return 0;
}

static int cube_bound_get_cube(const SylvesBound* bound, int* min_x, int* min_y, int* min_z,
                               int* max_x, int* max_y, int* max_z) {
    const CubeBound* cb = (const CubeBound*)bound;
    if (min_x) *min_x = cb->min_x;
    if (min_y) *min_y = cb->min_y;
    if (min_z) *min_z = cb->min_z;
    if (max_x) *max_x = cb->max_x;
    if (max_y) *max_y = cb->max_y;
    if (max_z) *max_z = cb->max_z;
    return 0;
}

static int cube_bound_get_cells(const SylvesBound* bound, SylvesCell* cells, size_t max_cells) {
    const CubeBound* cb = (const CubeBound*)bound;
    
    /* Calculate total number of cells */
    size_t width = (size_t)(cb->max_x - cb->min_x + 1);
    size_t height = (size_t)(cb->max_y - cb->min_y + 1);
    size_t depth = (size_t)(cb->max_z - cb->min_z + 1);
    size_t total = width * height * depth;
    
    /* Check for overflow */
    if (width > 0 && height > 0 && depth > 0) {
        if (total / width != height * depth) {
            return -1;  // Overflow
        }
    }
    
    /* Return count if no array provided */
    if (!cells) {
        return (int)total;
    }
    
    /* Fill array up to max_cells */
    size_t idx = 0;
    for (int z = cb->min_z; z <= cb->max_z && idx < max_cells; z++) {
        for (int y = cb->min_y; y <= cb->max_y && idx < max_cells; y++) {
            for (int x = cb->min_x; x <= cb->max_x && idx < max_cells; x++) {
                cells[idx++] = (SylvesCell){x, y, z};
            }
        }
    }
    
    return (int)idx;
}

/* Public API */
SylvesBound* sylves_cube_bound_create(int min_x, int min_y, int min_z,
                                      int max_x, int max_y, int max_z) {
    if (min_x > max_x || min_y > max_y || min_z > max_z) {
        return NULL;
    }
    
    CubeBound* bound = (CubeBound*)sylves_alloc(sizeof(CubeBound));
    if (!bound) {
        return NULL;
    }
    
    bound->base.vtable = &cube_bound_vtable;
    bound->base.type = SYLVES_BOUND_CUBE;
    bound->base.data = bound;
    
    bound->min_x = min_x;
    bound->min_y = min_y;
    bound->min_z = min_z;
    bound->max_x = max_x;
    bound->max_y = max_y;
    bound->max_z = max_z;
    
    return &bound->base;
}

int sylves_cube_bound_get_min_x(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->min_x;
}

int sylves_cube_bound_get_min_y(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->min_y;
}

int sylves_cube_bound_get_min_z(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->min_z;
}

int sylves_cube_bound_get_max_x(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->max_x;
}

int sylves_cube_bound_get_max_y(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->max_y;
}

int sylves_cube_bound_get_max_z(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_CUBE) {
        return 0;
    }
    const CubeBound* cube_bound = (const CubeBound*)bound->data;
    return cube_bound->max_z;
}
