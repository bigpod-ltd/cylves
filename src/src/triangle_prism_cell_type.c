/**
 * @file triangle_prism_cell_type.c
 * @brief Triangle prism cell type implementation
 */

#include "sylves/triangle_prism_cell_type.h"
#include "internal/cell_type_internal.h"
#include "sylves/vector.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    bool flat_topped;
} TrianglePrismCellData;

/* Forward declarations */
static int triangle_prism_get_dimension(const SylvesCellType* ct);
static int triangle_prism_get_dir_count(const SylvesCellType* ct);
static int triangle_prism_get_corner_count(const SylvesCellType* ct);
static SylvesVector3 triangle_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c);
static const char* triangle_prism_name(const SylvesCellType* ct);
static void triangle_prism_destroy(SylvesCellType* ct);

/* VTable for triangle prism cell type */
static const SylvesCellTypeVTable triangle_prism_vtable = {
    .get_dimension = triangle_prism_get_dimension,
    .get_dir_count = triangle_prism_get_dir_count,
    .get_corner_count = triangle_prism_get_corner_count,
    .get_corner_pos = triangle_prism_get_corner_pos,
    .name = triangle_prism_name,
    .destroy = triangle_prism_destroy
};

/* Static instances for flat-topped and flat-sided */
static SylvesCellType ft_triangle_prism_instance = {
    .vtable = &triangle_prism_vtable,
    .data = NULL
};

static SylvesCellType fs_triangle_prism_instance = {
    .vtable = &triangle_prism_vtable,
    .data = NULL
};

static TrianglePrismCellData ft_data = { .flat_topped = true };
static TrianglePrismCellData fs_data = { .flat_topped = false };

/* Initialize the static instances */
static void init_instances(void) {
    static bool initialized = false;
    if (!initialized) {
        ft_triangle_prism_instance.data = &ft_data;
        fs_triangle_prism_instance.data = &fs_data;
        initialized = true;
    }
}

const SylvesCellType* sylves_triangle_prism_cell_type_get(bool flat_topped) {
    init_instances();
    return flat_topped ? &ft_triangle_prism_instance : &fs_triangle_prism_instance;
}

/* Implementation functions */
static int triangle_prism_get_dimension(const SylvesCellType* ct) {
    return 3; /* Triangle prisms are 3D */
}

static int triangle_prism_get_dir_count(const SylvesCellType* ct) {
    return 8; /* 6 hex-like dirs + 2 vertical (forward/back) */
}

static int triangle_prism_get_corner_count(const SylvesCellType* ct) {
    return 6; /* 3 corners on top + 3 on bottom */
}

static SylvesVector3 triangle_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c) {
    TrianglePrismCellData* data = (TrianglePrismCellData*)ct->data;
    
    int flat_corner = c % 3;
    bool is_top = c >= 3;
    
    /* Get the 2D position of the corner on the triangle face */
    SylvesVector3 pos;
    
    if (data->flat_topped) {
        /* Flat-topped triangle corners */
        switch (flat_corner) {
            case 0: /* Bottom left */
                pos.x = -0.5;
                pos.y = -0.28867513459481288; /* -1/(2*sqrt(3)) */
                break;
            case 1: /* Bottom right */
                pos.x = 0.5;
                pos.y = -0.28867513459481288;
                break;
            case 2: /* Top */
                pos.x = 0.0;
                pos.y = 0.57735026918962576; /* 1/sqrt(3) */
                break;
        }
    } else {
        /* Flat-sided triangle corners (rotated 90 degrees) */
        switch (flat_corner) {
            case 0: /* Left */
                pos.x = -0.57735026918962576; /* -1/sqrt(3) */
                pos.y = 0.0;
                break;
            case 1: /* Bottom right */
                pos.x = 0.28867513459481288; /* 1/(2*sqrt(3)) */
                pos.y = -0.5;
                break;
            case 2: /* Top right */
                pos.x = 0.28867513459481288;
                pos.y = 0.5;
                break;
        }
    }
    
    pos.z = is_top ? 0.5 : -0.5;
    
    return pos;
}

static const char* triangle_prism_name(const SylvesCellType* ct) {
    TrianglePrismCellData* data = (TrianglePrismCellData*)ct->data;
    return data->flat_topped ? "FlatToppedTrianglePrism" : "FlatSidedTrianglePrism";
}

static void triangle_prism_destroy(SylvesCellType* ct) {
    /* Static instances, nothing to free */
}
