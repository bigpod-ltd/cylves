/**
 * @file ngon_prism_cell_type.c
 * @brief N-gon prism cell type implementation
 */

#include "sylves/ngon_prism_cell_type.h"
#include "internal/cell_type_internal.h"
#include "sylves/vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_CACHED_NGONS 20

typedef struct {
    int n; /* Number of sides */
} NGonPrismCellData;

/* Forward declarations */
static int ngon_prism_get_dimension(const SylvesCellType* ct);
static int ngon_prism_get_dir_count(const SylvesCellType* ct);
static int ngon_prism_get_corner_count(const SylvesCellType* ct);
static SylvesVector3 ngon_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c);
static const char* ngon_prism_name(const SylvesCellType* ct);
static void ngon_prism_destroy(SylvesCellType* ct);

/* VTable for n-gon prism cell type */
static const SylvesCellTypeVTable ngon_prism_vtable = {
    .get_dimension = ngon_prism_get_dimension,
    .get_dir_count = ngon_prism_get_dir_count,
    .get_corner_count = ngon_prism_get_corner_count,
    .get_corner_pos = ngon_prism_get_corner_pos,
    .name = ngon_prism_name,
    .destroy = ngon_prism_destroy
};

/* Cache of cell type instances for common n values */
static SylvesCellType* cached_instances[MAX_CACHED_NGONS] = {NULL};
static NGonPrismCellData cached_data[MAX_CACHED_NGONS];

const SylvesCellType* sylves_ngon_prism_cell_type_get(int n) {
    if (n < 3) {
        return NULL; /* Invalid n-gon */
    }
    
    /* For small n values, use cached instances */
    if (n < MAX_CACHED_NGONS) {
        if (!cached_instances[n]) {
            /* Create and cache the instance */
            cached_instances[n] = malloc(sizeof(SylvesCellType));
            if (!cached_instances[n]) {
                return NULL;
            }
            cached_instances[n]->vtable = &ngon_prism_vtable;
            cached_data[n].n = n;
            cached_instances[n]->data = &cached_data[n];
        }
        return cached_instances[n];
    }
    
    /* For larger n values, create a new instance */
    SylvesCellType* ct = malloc(sizeof(SylvesCellType));
    if (!ct) {
        return NULL;
    }
    
    NGonPrismCellData* data = malloc(sizeof(NGonPrismCellData));
    if (!data) {
        free(ct);
        return NULL;
    }
    
    data->n = n;
    ct->vtable = &ngon_prism_vtable;
    ct->data = data;
    
    return ct;
}

/* Implementation functions */
static int ngon_prism_get_dimension(const SylvesCellType* ct) {
    return 3; /* N-gon prisms are 3D */
}

static int ngon_prism_get_dir_count(const SylvesCellType* ct) {
    NGonPrismCellData* data = (NGonPrismCellData*)ct->data;
    return data->n + 2; /* n base dirs + 2 vertical (forward/back) */
}

static int ngon_prism_get_corner_count(const SylvesCellType* ct) {
    NGonPrismCellData* data = (NGonPrismCellData*)ct->data;
    return data->n * 2; /* n corners on top + n on bottom */
}

static SylvesVector3 ngon_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c) {
    NGonPrismCellData* data = (NGonPrismCellData*)ct->data;
    
    int flat_corner = c % data->n;
    bool is_top = c >= data->n;
    
    /* Get the 2D position of the corner on the n-gon face */
    double angle = (2.0 * M_PI * flat_corner) / data->n;
    
    SylvesVector3 pos;
    pos.x = 0.5 * cos(angle);
    pos.y = 0.5 * sin(angle);
    pos.z = is_top ? 0.5 : -0.5;
    
    return pos;
}

static const char* ngon_prism_name(const SylvesCellType* ct) {
    NGonPrismCellData* data = (NGonPrismCellData*)ct->data;
    static char name[32];
    snprintf(name, sizeof(name), "%d-gonPrism", data->n);
    return name;
}

static void ngon_prism_destroy(SylvesCellType* ct) {
    if (!ct) return;
    
    NGonPrismCellData* data = (NGonPrismCellData*)ct->data;
    if (!data) return;
    
    /* Check if this is a cached instance */
    if (data->n < MAX_CACHED_NGONS && ct == cached_instances[data->n]) {
        /* Don't free cached instances */
        return;
    }
    
    /* Free non-cached instances */
    free(data);
    free(ct);
}
