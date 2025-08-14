/**
 * @file cell_type_internal.h
 * @brief Internal cell type structure and vtable
 */

#ifndef CELL_TYPE_INTERNAL_H
#define CELL_TYPE_INTERNAL_H

#include "sylves/types.h"


/* Virtual function table for cell type operations */
typedef struct {
    /* Properties */
    int  (*get_dimension)(const SylvesCellType* ct);      /* 2 or 3 */
    int  (*get_dir_count)(const SylvesCellType* ct);      /* number of directions */
    int  (*get_corner_count)(const SylvesCellType* ct);   /* number of corners */

    /* Geometry helpers (optional) */
    SylvesVector3 (*get_corner_pos)(const SylvesCellType* ct, SylvesCellCorner c);

    /* Index helpers (optional) */
    const char* (*name)(const SylvesCellType* ct);

    /* Lifetime */
    void (*destroy)(SylvesCellType* ct);
} SylvesCellTypeVTable;

/* Base cell type structure */
struct SylvesCellType {
    const SylvesCellTypeVTable* vtable;
    void* data; /* implementation specific */
};

/* Default helpers derived from the vtable */
static inline bool sylves_ct_is_2d(const SylvesCellType* ct) {
    if (!ct || !ct->vtable) return false;
    if (ct->vtable->get_dimension) return ct->vtable->get_dimension(ct) == 2;
    return false;
}

static inline bool sylves_ct_is_3d(const SylvesCellType* ct) {
    if (!ct || !ct->vtable) return false;
    if (ct->vtable->get_dimension) return ct->vtable->get_dimension(ct) == 3;
    return false;
}

static inline int sylves_ct_dir_count(const SylvesCellType* ct) {
    if (!ct || !ct->vtable || !ct->vtable->get_dir_count) return 0;
    return ct->vtable->get_dir_count(ct);
}

static inline int sylves_ct_corner_count(const SylvesCellType* ct) {
    if (!ct || !ct->vtable || !ct->vtable->get_corner_count) return 0;
    return ct->vtable->get_corner_count(ct);
}


#endif /* CELL_TYPE_INTERNAL_H */

