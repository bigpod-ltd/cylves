/**
 * @file mask_bound.c
 * @brief MaskBound implementation for arbitrary cell sets
 */

#include "sylves/mask_bound.h"
#include "sylves/memory.h"
#include "internal/bound_internal.h"
#include <stdlib.h>
#include <string.h>

/* Hash table entry for cell storage */
typedef struct CellNode {
    SylvesCell cell;
    struct CellNode* next;
} CellNode;

/* MaskBound data structure */
typedef struct {
    CellNode** buckets;     /* Hash table buckets */
    size_t bucket_count;    /* Number of buckets */
    size_t cell_count;      /* Number of cells in bound */
    SylvesCell* cell_array; /* Optional array for fast enumeration */
    bool array_valid;       /* Whether cell_array is up to date */
} MaskBoundData;

/* Hash function for cells */
static size_t cell_hash(SylvesCell cell, size_t bucket_count) {
    /* Simple hash combining x, y, z coordinates */
    size_t hash = (size_t)cell.x;
    hash = hash * 31 + (size_t)cell.y;
    hash = hash * 31 + (size_t)cell.z;
    return hash % bucket_count;
}

/* Find cell in hash table */
static bool mask_find_cell(const MaskBoundData* data, SylvesCell cell) {
    size_t index = cell_hash(cell, data->bucket_count);
    CellNode* node = data->buckets[index];
    
    while (node) {
        if (node->cell.x == cell.x && node->cell.y == cell.y && node->cell.z == cell.z) {
            return true;
        }
        node = node->next;
    }
    return false;
}

/* Add cell to hash table */
static bool mask_add_cell(MaskBoundData* data, SylvesCell cell) {
    if (mask_find_cell(data, cell)) {
        return false; /* Already exists */
    }
    
    size_t index = cell_hash(cell, data->bucket_count);
    CellNode* node = (CellNode*)sylves_alloc(sizeof(CellNode));
    if (!node) return false;
    
    node->cell = cell;
    node->next = data->buckets[index];
    data->buckets[index] = node;
    data->cell_count++;
    data->array_valid = false;
    return true;
}

/* Remove cell from hash table */
static bool mask_remove_cell(MaskBoundData* data, SylvesCell cell) {
    size_t index = cell_hash(cell, data->bucket_count);
    CellNode* node = data->buckets[index];
    CellNode* prev = NULL;
    
    while (node) {
        if (node->cell.x == cell.x && node->cell.y == cell.y && node->cell.z == cell.z) {
            if (prev) {
                prev->next = node->next;
            } else {
                data->buckets[index] = node->next;
            }
            sylves_free(node);
            data->cell_count--;
            data->array_valid = false;
            return true;
        }
        prev = node;
        node = node->next;
    }
    return false;
}

/* Build cell array for enumeration */
static void mask_build_array(MaskBoundData* data) {
    if (data->array_valid) return;
    
    if (data->cell_array) {
        sylves_free(data->cell_array);
    }
    
    if (data->cell_count == 0) {
        data->cell_array = NULL;
        data->array_valid = true;
        return;
    }
    
    data->cell_array = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * data->cell_count);
    if (!data->cell_array) return;
    
    size_t index = 0;
    for (size_t i = 0; i < data->bucket_count; i++) {
        CellNode* node = data->buckets[i];
        while (node) {
            data->cell_array[index++] = node->cell;
            node = node->next;
        }
    }
    data->array_valid = true;
}

/* VTable functions */
static bool mask_contains(const SylvesBound* b, SylvesCell c) {
    const MaskBoundData* data = (const MaskBoundData*)b->data;
    return mask_find_cell(data, c);
}

static void mask_destroy(SylvesBound* b) {
    if (!b) return;
    
    MaskBoundData* data = (MaskBoundData*)b->data;
    if (data) {
        /* Free all nodes */
        for (size_t i = 0; i < data->bucket_count; i++) {
            CellNode* node = data->buckets[i];
            while (node) {
                CellNode* next = node->next;
                sylves_free(node);
                node = next;
            }
        }
        
        if (data->buckets) sylves_free(data->buckets);
        if (data->cell_array) sylves_free(data->cell_array);
        sylves_free(data);
    }
    sylves_free(b);
}

static const char* mask_name(const SylvesBound* b) {
    (void)b;
    return "mask";
}

static int mask_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    MaskBoundData* data = (MaskBoundData*)b->data;
    mask_build_array(data);
    
    if (!data->cell_array) return 0;
    
    size_t count = data->cell_count < max_cells ? data->cell_count : max_cells;
    if (cells) {
        memcpy(cells, data->cell_array, sizeof(SylvesCell) * count);
    }
    return (int)count;
}

static int mask_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    MaskBoundData* data = (MaskBoundData*)b->data;
    if (data->cell_count == 0) return -1;
    
    mask_build_array(data);
    if (!data->cell_array) return -1;
    
    int minx = data->cell_array[0].x, maxx = data->cell_array[0].x;
    int miny = data->cell_array[0].y, maxy = data->cell_array[0].y;
    
    for (size_t i = 1; i < data->cell_count; i++) {
        if (data->cell_array[i].x < minx) minx = data->cell_array[i].x;
        if (data->cell_array[i].x > maxx) maxx = data->cell_array[i].x;
        if (data->cell_array[i].y < miny) miny = data->cell_array[i].y;
        if (data->cell_array[i].y > maxy) maxy = data->cell_array[i].y;
    }
    
    if (min_x) *min_x = minx;
    if (min_y) *min_y = miny;
    if (max_x) *max_x = maxx;
    if (max_y) *max_y = maxy;
    return 0;
}

static int mask_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                        int* max_x, int* max_y, int* max_z) {
    MaskBoundData* data = (MaskBoundData*)b->data;
    if (data->cell_count == 0) return -1;
    
    mask_build_array(data);
    if (!data->cell_array) return -1;
    
    int minx = data->cell_array[0].x, maxx = data->cell_array[0].x;
    int miny = data->cell_array[0].y, maxy = data->cell_array[0].y;
    int minz = data->cell_array[0].z, maxz = data->cell_array[0].z;
    
    for (size_t i = 1; i < data->cell_count; i++) {
        if (data->cell_array[i].x < minx) minx = data->cell_array[i].x;
        if (data->cell_array[i].x > maxx) maxx = data->cell_array[i].x;
        if (data->cell_array[i].y < miny) miny = data->cell_array[i].y;
        if (data->cell_array[i].y > maxy) maxy = data->cell_array[i].y;
        if (data->cell_array[i].z < minz) minz = data->cell_array[i].z;
        if (data->cell_array[i].z > maxz) maxz = data->cell_array[i].z;
    }
    
    if (min_x) *min_x = minx;
    if (min_y) *min_y = miny;
    if (min_z) *min_z = minz;
    if (max_x) *max_x = maxx;
    if (max_y) *max_y = maxy;
    if (max_z) *max_z = maxz;
    return 0;
}

static SylvesBound* mask_intersect(const SylvesBound* a, const SylvesBound* b) {
    /* For mask bounds, intersection is cells in both sets */
    if (a->type != SYLVES_BOUND_TYPE_MASK || b->type != SYLVES_BOUND_TYPE_MASK) {
        return NULL; /* Can't handle mixed types here */
    }
    
    const MaskBoundData* data_a = (const MaskBoundData*)a->data;
    const MaskBoundData* data_b = (const MaskBoundData*)b->data;
    
    /* Use smaller set to iterate */
    const MaskBoundData* smaller = data_a->cell_count < data_b->cell_count ? data_a : data_b;
    const SylvesBound* larger = data_a->cell_count < data_b->cell_count ? b : a;
    
    /* Collect cells in both sets */
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * smaller->cell_count);
    if (!cells) return NULL;
    
    size_t count = 0;
    for (size_t i = 0; i < smaller->bucket_count; i++) {
        CellNode* node = smaller->buckets[i];
        while (node) {
            if (sylves_bound_contains(larger, node->cell)) {
                cells[count++] = node->cell;
            }
            node = node->next;
        }
    }
    
    SylvesBound* result = sylves_bound_create_mask(cells, count);
    sylves_free(cells);
    return result;
}

static SylvesBound* mask_union(const SylvesBound* a, const SylvesBound* b) {
    /* For mask bounds, union is all cells in either set */
    if (a->type != SYLVES_BOUND_TYPE_MASK || b->type != SYLVES_BOUND_TYPE_MASK) {
        return NULL; /* Can't handle mixed types here */
    }
    
    MaskBoundData* data_a = (MaskBoundData*)a->data;
    MaskBoundData* data_b = (MaskBoundData*)b->data;
    
    mask_build_array(data_a);
    mask_build_array(data_b);
    
    /* Create result with all cells from a */
    SylvesBound* result = sylves_bound_create_mask(data_a->cell_array, data_a->cell_count);
    if (!result) return NULL;
    
    /* Add all cells from b */
    sylves_mask_bound_add_cells(result, data_b->cell_array, data_b->cell_count);
    
    return result;
}

static int mask_get_cell_count(const SylvesBound* b) {
    const MaskBoundData* data = (const MaskBoundData*)b->data;
    return (int)data->cell_count;
}

static SylvesBound* mask_clone(const SylvesBound* b) {
    MaskBoundData* data = (MaskBoundData*)b->data;
    mask_build_array(data);
    return sylves_bound_create_mask(data->cell_array, data->cell_count);
}

static bool mask_is_empty(const SylvesBound* b) {
    const MaskBoundData* data = (const MaskBoundData*)b->data;
    return data->cell_count == 0;
}

static int mask_get_aabb(const SylvesBound* b, float* min, float* max) {
    /* Get integer bounds and convert to float */
    int min_x, min_y, min_z, max_x, max_y, max_z;
    if (mask_get_cube(b, &min_x, &min_y, &min_z, &max_x, &max_y, &max_z) != 0) {
        return -1;
    }
    
    if (min) {
        min[0] = (float)min_x;
        min[1] = (float)min_y;
        min[2] = (float)min_z;
    }
    if (max) {
        max[0] = (float)(max_x + 1);
        max[1] = (float)(max_y + 1);
        max[2] = (float)(max_z + 1);
    }
    return 0;
}

static const SylvesBoundVTable MASK_VT = {
    .contains = mask_contains,
    .destroy = mask_destroy,
    .name = mask_name,
    .get_cells = mask_get_cells,
    .get_rect = mask_get_rect,
    .get_cube = mask_get_cube,
    .intersect = mask_intersect,
    .union_bounds = mask_union,
    .get_cell_count = mask_get_cell_count,
    .clone = mask_clone,
    .is_empty = mask_is_empty,
    .get_aabb = mask_get_aabb
};

/* Public API */
SylvesBound* sylves_bound_create_mask(const SylvesCell* cells, size_t cell_count) {
    /* Choose bucket count based on expected size */
    size_t bucket_count = 16;
    while (bucket_count < cell_count / 2) {
        bucket_count *= 2;
    }
    
    SylvesBound* bound = (SylvesBound*)sylves_alloc(sizeof(SylvesBound));
    if (!bound) return NULL;
    
    MaskBoundData* data = (MaskBoundData*)sylves_alloc(sizeof(MaskBoundData));
    if (!data) {
        sylves_free(bound);
        return NULL;
    }
    
    data->buckets = (CellNode**)sylves_calloc(bucket_count, sizeof(CellNode*));
    if (!data->buckets) {
        sylves_free(data);
        sylves_free(bound);
        return NULL;
    }
    
    data->bucket_count = bucket_count;
    data->cell_count = 0;
    data->cell_array = NULL;
    data->array_valid = false;
    
    bound->vtable = &MASK_VT;
    bound->data = data;
    bound->type = SYLVES_BOUND_TYPE_MASK;
    
    /* Add all cells */
    for (size_t i = 0; i < cell_count; i++) {
        mask_add_cell(data, cells[i]);
    }
    
    return bound;
}

SylvesBound* sylves_bound_create_mask_filtered(
    const SylvesBound* base,
    bool (*filter)(SylvesCell cell, void* user_data),
    void* user_data) {
    
    if (!base || !filter) return NULL;
    
    /* Get max cells from base */
    int count = sylves_bound_get_cells(base, NULL, 0);
    if (count <= 0) return sylves_bound_create_mask(NULL, 0);
    
    SylvesCell* cells = (SylvesCell*)sylves_alloc(sizeof(SylvesCell) * count);
    if (!cells) return NULL;
    
    count = sylves_bound_get_cells(base, cells, count);
    
    /* Filter cells */
    size_t filtered_count = 0;
    for (int i = 0; i < count; i++) {
        if (filter(cells[i], user_data)) {
            cells[filtered_count++] = cells[i];
        }
    }
    
    SylvesBound* result = sylves_bound_create_mask(cells, filtered_count);
    sylves_free(cells);
    return result;
}

int sylves_mask_bound_add_cells(SylvesBound* bound, const SylvesCell* cells, size_t cell_count) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_MASK) return -1;
    
    MaskBoundData* data = (MaskBoundData*)bound->data;
    for (size_t i = 0; i < cell_count; i++) {
        mask_add_cell(data, cells[i]);
    }
    return 0;
}

int sylves_mask_bound_remove_cells(SylvesBound* bound, const SylvesCell* cells, size_t cell_count) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_MASK) return -1;
    
    MaskBoundData* data = (MaskBoundData*)bound->data;
    for (size_t i = 0; i < cell_count; i++) {
        mask_remove_cell(data, cells[i]);
    }
    return 0;
}

int sylves_mask_bound_get_count(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_MASK) return -1;
    
    const MaskBoundData* data = (const MaskBoundData*)bound->data;
    return (int)data->cell_count;
}

int sylves_mask_bound_clear(SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_MASK) return -1;
    
    MaskBoundData* data = (MaskBoundData*)bound->data;
    
    /* Free all nodes */
    for (size_t i = 0; i < data->bucket_count; i++) {
        CellNode* node = data->buckets[i];
        while (node) {
            CellNode* next = node->next;
            sylves_free(node);
            node = next;
        }
        data->buckets[i] = NULL;
    }
    
    data->cell_count = 0;
    data->array_valid = false;
    if (data->cell_array) {
        sylves_free(data->cell_array);
        data->cell_array = NULL;
    }
    
    return 0;
}
