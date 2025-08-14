/**
 * @file aabb_bound.c
 * @brief AabbBound implementation for axis-aligned bounding box constraints
 */

#include "sylves/aabb_bound.h"
#include "sylves/memory.h"
#include "sylves/grid.h"
#include "internal/bound_internal.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>

/* AabbBound data structure */
typedef struct {
    float min[3];
    float max[3];
    int dimensions; /* 2 or 3 */
} AabbBoundData;

/* VTable functions */
static bool aabb_contains(const SylvesBound* b, SylvesCell c) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    /* Check if cell center is within AABB */
    float x = (float)c.x + 0.5f;
    float y = (float)c.y + 0.5f;
    float z = (float)c.z + 0.5f;
    
    if (x < data->min[0] || x > data->max[0]) return false;
    if (y < data->min[1] || y > data->max[1]) return false;
    
    if (data->dimensions == 3) {
        if (z < data->min[2] || z > data->max[2]) return false;
    } else {
        if (c.z != 0) return false; /* 2D bound only contains z=0 cells */
    }
    
    return true;
}

static void aabb_destroy(SylvesBound* b) {
    if (!b) return;
    if (b->data) sylves_free(b->data);
    sylves_free(b);
}

static const char* aabb_name(const SylvesBound* b) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    return data->dimensions == 3 ? "aabb3d" : "aabb2d";
}

static int aabb_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    /* Get integer bounds */
    int min_x = (int)floorf(data->min[0]);
    int min_y = (int)floorf(data->min[1]);
    int min_z = data->dimensions == 3 ? (int)floorf(data->min[2]) : 0;
    int max_x = (int)ceilf(data->max[0]);
    int max_y = (int)ceilf(data->max[1]);
    int max_z = data->dimensions == 3 ? (int)ceilf(data->max[2]) : 0;
    
    size_t count = 0;
    
    if (data->dimensions == 3) {
        for (int z = min_z; z <= max_z && count < max_cells; z++) {
            for (int y = min_y; y <= max_y && count < max_cells; y++) {
                for (int x = min_x; x <= max_x && count < max_cells; x++) {
                    SylvesCell cell = {x, y, z};
                    if (aabb_contains(b, cell)) {
                        if (cells) cells[count] = cell;
                        count++;
                    }
                }
            }
        }
    } else {
        for (int y = min_y; y <= max_y && count < max_cells; y++) {
            for (int x = min_x; x <= max_x && count < max_cells; x++) {
                SylvesCell cell = {x, y, 0};
                if (aabb_contains(b, cell)) {
                    if (cells) cells[count] = cell;
                    count++;
                }
            }
        }
    }
    
    return (int)count;
}

static int aabb_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    if (min_x) *min_x = (int)floorf(data->min[0]);
    if (min_y) *min_y = (int)floorf(data->min[1]);
    if (max_x) *max_x = (int)ceilf(data->max[0]) - 1;
    if (max_y) *max_y = (int)ceilf(data->max[1]) - 1;
    
    return 0;
}

static int aabb_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                        int* max_x, int* max_y, int* max_z) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    if (min_x) *min_x = (int)floorf(data->min[0]);
    if (min_y) *min_y = (int)floorf(data->min[1]);
    if (min_z) *min_z = data->dimensions == 3 ? (int)floorf(data->min[2]) : 0;
    if (max_x) *max_x = (int)ceilf(data->max[0]) - 1;
    if (max_y) *max_y = (int)ceilf(data->max[1]) - 1;
    if (max_z) *max_z = data->dimensions == 3 ? (int)ceilf(data->max[2]) - 1 : 0;
    
    return 0;
}

static SylvesBound* aabb_intersect(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_AABB || b->type != SYLVES_BOUND_TYPE_AABB) {
        return NULL;
    }
    
    const AabbBoundData* data_a = (const AabbBoundData*)a->data;
    const AabbBoundData* data_b = (const AabbBoundData*)b->data;
    
    /* Both must have same dimensions */
    if (data_a->dimensions != data_b->dimensions) return NULL;
    
    /* Compute intersection */
    float min[3], max[3];
    for (int i = 0; i < 3; i++) {
        min[i] = fmaxf(data_a->min[i], data_b->min[i]);
        max[i] = fminf(data_a->max[i], data_b->max[i]);
        
        /* Empty intersection */
        if (min[i] > max[i]) {
            if (data_a->dimensions == 3) {
                return sylves_bound_create_aabb_3d(1, 1, 1, 0, 0, 0);
            } else {
                return sylves_bound_create_aabb_2d(1, 1, 0, 0);
            }
        }
    }
    
    if (data_a->dimensions == 3) {
        return sylves_bound_create_aabb_3d(min[0], min[1], min[2], max[0], max[1], max[2]);
    } else {
        return sylves_bound_create_aabb_2d(min[0], min[1], max[0], max[1]);
    }
}

static SylvesBound* aabb_union(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_AABB || b->type != SYLVES_BOUND_TYPE_AABB) {
        return NULL;
    }
    
    const AabbBoundData* data_a = (const AabbBoundData*)a->data;
    const AabbBoundData* data_b = (const AabbBoundData*)b->data;
    
    /* Both must have same dimensions */
    if (data_a->dimensions != data_b->dimensions) return NULL;
    
    /* Compute union */
    float min[3], max[3];
    for (int i = 0; i < 3; i++) {
        min[i] = fminf(data_a->min[i], data_b->min[i]);
        max[i] = fmaxf(data_a->max[i], data_b->max[i]);
    }
    
    if (data_a->dimensions == 3) {
        return sylves_bound_create_aabb_3d(min[0], min[1], min[2], max[0], max[1], max[2]);
    } else {
        return sylves_bound_create_aabb_2d(min[0], min[1], max[0], max[1]);
    }
}

static int aabb_get_cell_count(const SylvesBound* b) {
    /* AABB can contain infinite cells in general */
    return -1;
}

static SylvesBound* aabb_clone(const SylvesBound* b) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    if (data->dimensions == 3) {
        return sylves_bound_create_aabb_3d(data->min[0], data->min[1], data->min[2],
                                          data->max[0], data->max[1], data->max[2]);
    } else {
        return sylves_bound_create_aabb_2d(data->min[0], data->min[1],
                                          data->max[0], data->max[1]);
    }
}

static bool aabb_is_empty(const SylvesBound* b) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    for (int i = 0; i < data->dimensions; i++) {
        if (data->min[i] >= data->max[i]) return true;
    }
    return false;
}

static int aabb_get_aabb(const SylvesBound* b, float* min, float* max) {
    const AabbBoundData* data = (const AabbBoundData*)b->data;
    
    if (min) {
        min[0] = data->min[0];
        min[1] = data->min[1];
        min[2] = data->min[2];
    }
    if (max) {
        max[0] = data->max[0];
        max[1] = data->max[1];
        max[2] = data->max[2];
    }
    return 0;
}

static const SylvesBoundVTable AABB_VT = {
    .contains = aabb_contains,
    .destroy = aabb_destroy,
    .name = aabb_name,
    .get_cells = aabb_get_cells,
    .get_rect = aabb_get_rect,
    .get_cube = aabb_get_cube,
    .intersect = aabb_intersect,
    .union_bounds = aabb_union,
    .get_cell_count = aabb_get_cell_count,
    .clone = aabb_clone,
    .is_empty = aabb_is_empty,
    .get_aabb = aabb_get_aabb
};

/* Public API */
SylvesBound* sylves_bound_create_aabb_2d(float min_x, float min_y, float max_x, float max_y) {
    SylvesBound* bound = (SylvesBound*)sylves_alloc(sizeof(SylvesBound));
    if (!bound) return NULL;
    
    AabbBoundData* data = (AabbBoundData*)sylves_alloc(sizeof(AabbBoundData));
    if (!data) {
        sylves_free(bound);
        return NULL;
    }
    
    data->min[0] = min_x;
    data->min[1] = min_y;
    data->min[2] = 0.0f;
    data->max[0] = max_x;
    data->max[1] = max_y;
    data->max[2] = 0.0f;
    data->dimensions = 2;
    
    bound->vtable = &AABB_VT;
    bound->data = data;
    bound->type = SYLVES_BOUND_TYPE_AABB;
    
    return bound;
}

SylvesBound* sylves_bound_create_aabb_3d(float min_x, float min_y, float min_z,
                                         float max_x, float max_y, float max_z) {
    SylvesBound* bound = (SylvesBound*)sylves_alloc(sizeof(SylvesBound));
    if (!bound) return NULL;
    
    AabbBoundData* data = (AabbBoundData*)sylves_alloc(sizeof(AabbBoundData));
    if (!data) {
        sylves_free(bound);
        return NULL;
    }
    
    data->min[0] = min_x;
    data->min[1] = min_y;
    data->min[2] = min_z;
    data->max[0] = max_x;
    data->max[1] = max_y;
    data->max[2] = max_z;
    data->dimensions = 3;
    
    bound->vtable = &AABB_VT;
    bound->data = data;
    bound->type = SYLVES_BOUND_TYPE_AABB;
    
    return bound;
}

SylvesBound* sylves_bound_create_aabb_from_cells(const SylvesGrid* grid, const SylvesBound* cell_bound) {
    if (!grid || !cell_bound) return NULL;
    
    /* Get cells from bound */
    int count = sylves_bound_get_cells(cell_bound, NULL, 0);
    if (count <= 0) return NULL;
    
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * count);
    if (!cells) return NULL;
    
    count = sylves_bound_get_cells(cell_bound, cells, count);
    
    /* Initialize bounds */
    float min[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float max[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    bool is_3d = sylves_grid_is_3d(grid);
    
    /* Compute AABB from cell AABBs */
    for (int i = 0; i < count; i++) {
        SylvesAabb cell_aabb;
        if (sylves_grid_get_cell_aabb(grid, cells[i], &cell_aabb) == 0) {
            min[0] = fminf(min[0], cell_aabb.min.x);
            min[1] = fminf(min[1], cell_aabb.min.y);
            min[2] = fminf(min[2], cell_aabb.min.z);
            max[0] = fmaxf(max[0], cell_aabb.max.x);
            max[1] = fmaxf(max[1], cell_aabb.max.y);
            max[2] = fmaxf(max[2], cell_aabb.max.z);
        }
    }
    
    sylves_free(cells);
    
    /* Check if we got valid bounds */
    if (min[0] > max[0]) return NULL;
    
    if (is_3d) {
        return sylves_bound_create_aabb_3d(min[0], min[1], min[2], max[0], max[1], max[2]);
    } else {
        return sylves_bound_create_aabb_2d(min[0], min[1], max[0], max[1]);
    }
}

int sylves_aabb_bound_get_bounds(const SylvesBound* bound, float* min, float* max) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_AABB) return -1;
    
    const AabbBoundData* data = (const AabbBoundData*)bound->data;
    
    if (min) {
        min[0] = data->min[0];
        min[1] = data->min[1];
        min[2] = data->min[2];
    }
    if (max) {
        max[0] = data->max[0];
        max[1] = data->max[1];
        max[2] = data->max[2];
    }
    
    return 0;
}

bool sylves_aabb_bound_contains_point(const SylvesBound* bound, float x, float y, float z) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_AABB) return false;
    
    const AabbBoundData* data = (const AabbBoundData*)bound->data;
    
    if (x < data->min[0] || x > data->max[0]) return false;
    if (y < data->min[1] || y > data->max[1]) return false;
    
    if (data->dimensions == 3) {
        if (z < data->min[2] || z > data->max[2]) return false;
    }
    
    return true;
}

SylvesBound* sylves_aabb_bound_expand(const SylvesBound* bound, float margin) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_AABB) return NULL;
    
    const AabbBoundData* data = (const AabbBoundData*)bound->data;
    
    if (data->dimensions == 3) {
        return sylves_bound_create_aabb_3d(
            data->min[0] - margin, data->min[1] - margin, data->min[2] - margin,
            data->max[0] + margin, data->max[1] + margin, data->max[2] + margin);
    } else {
        return sylves_bound_create_aabb_2d(
            data->min[0] - margin, data->min[1] - margin,
            data->max[0] + margin, data->max[1] + margin);
    }
}

int sylves_aabb_bound_get_dimensions(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_AABB) return -1;
    
    const AabbBoundData* data = (const AabbBoundData*)bound->data;
    return data->dimensions;
}
