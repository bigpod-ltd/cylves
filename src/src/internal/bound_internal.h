/**
 * @file bound_internal.h
 * @brief Internal bound structure and vtable
 */

#ifndef BOUND_INTERNAL_H
#define BOUND_INTERNAL_H

#include "sylves/types.h"


/* Virtual function table for bounds */
typedef struct {
    bool (*contains)(const SylvesBound* b, SylvesCell c);
    void (*destroy)(SylvesBound* b);
    const char* (*name)(const SylvesBound* b);
    int  (*get_cells)(const SylvesBound* b, SylvesCell* cells, size_t max_cells);
    /* Rect extents: returns 0 on success, error otherwise */
    int  (*get_rect)(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y);
    /* Cube extents: returns 0 on success, error otherwise */
    int  (*get_cube)(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                     int* max_x, int* max_y, int* max_z);
    /* Intersect operation - returns new bound or NULL */
    SylvesBound* (*intersect)(const SylvesBound* a, const SylvesBound* b);
    /* Union operation - returns new bound or NULL */
    SylvesBound* (*union_bounds)(const SylvesBound* a, const SylvesBound* b);
    /* Get cell count - returns count or negative error */
    int (*get_cell_count)(const SylvesBound* b);
    /* Clone bound - returns copy or NULL */
    SylvesBound* (*clone)(const SylvesBound* b);
    /* Check if bound is empty */
    bool (*is_empty)(const SylvesBound* b);
    /* Get bounding AABB if applicable */
    int (*get_aabb)(const SylvesBound* b, float* min, float* max);
} SylvesBoundVTable;

/* Base bound structure */
struct SylvesBound {
    const SylvesBoundVTable* vtable;
    void* data; /* impl specific */
    int type;   /* 1=rect, 2=cube, etc. for quick querying */
};

static inline bool sylves_bound_call_contains(const SylvesBound* b, SylvesCell c) {
    if (!b || !b->vtable || !b->vtable->contains) return false;
    return b->vtable->contains(b, c);
}


#endif /* BOUND_INTERNAL_H */

