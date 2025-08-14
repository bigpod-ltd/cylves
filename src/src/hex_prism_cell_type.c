/**
 * @file hex_prism_cell_type.c
 * @brief Hex prism cell type implementation
 */

#include "sylves/hex_prism_cell_type.h"
#include "internal/cell_type_internal.h"
#include "sylves/vector.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    bool flat_topped;
} HexPrismCellData;

/* Forward declarations */
static int hex_prism_get_dimension(const SylvesCellType* ct);
static int hex_prism_get_dir_count(const SylvesCellType* ct);
static int hex_prism_get_corner_count(const SylvesCellType* ct);
static SylvesVector3 hex_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c);
static const char* hex_prism_name(const SylvesCellType* ct);
static void hex_prism_destroy(SylvesCellType* ct);

/* VTable for hex prism cell type */
static const SylvesCellTypeVTable hex_prism_vtable = {
    .get_dimension = hex_prism_get_dimension,
    .get_dir_count = hex_prism_get_dir_count,
    .get_corner_count = hex_prism_get_corner_count,
    .get_corner_pos = hex_prism_get_corner_pos,
    .name = hex_prism_name,
    .destroy = hex_prism_destroy
};

/* Static instances for flat-topped and pointy-topped */
static SylvesCellType ft_hex_prism_instance = {
    .vtable = &hex_prism_vtable,
    .data = NULL
};

static SylvesCellType pt_hex_prism_instance = {
    .vtable = &hex_prism_vtable,
    .data = NULL
};

static HexPrismCellData ft_data = { .flat_topped = true };
static HexPrismCellData pt_data = { .flat_topped = false };

/* Initialize the static instances */
static void init_instances(void) {
    static bool initialized = false;
    if (!initialized) {
        ft_hex_prism_instance.data = &ft_data;
        pt_hex_prism_instance.data = &pt_data;
        initialized = true;
    }
}

const SylvesCellType* sylves_hex_prism_cell_type_get(bool flat_topped) {
    init_instances();
    return flat_topped ? &ft_hex_prism_instance : &pt_hex_prism_instance;
}

/* Implementation functions */
static int hex_prism_get_dimension(const SylvesCellType* ct) {
    return 3; /* Hex prisms are 3D */
}

static int hex_prism_get_dir_count(const SylvesCellType* ct) {
    return 8; /* 6 hex dirs + 2 vertical (forward/back) */
}

static int hex_prism_get_corner_count(const SylvesCellType* ct) {
    return 12; /* 6 corners on top + 6 on bottom */
}

static SylvesVector3 hex_prism_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c) {
    HexPrismCellData* data = (HexPrismCellData*)ct->data;
    
    int flat_corner = c % 6;
    bool is_top = c >= 6;
    
    /* Get the 2D position of the corner on the hex face */
    double angle = flat_corner * M_PI / 3.0;
    if (data->flat_topped) {
        angle += M_PI / 6.0; /* 30 degree offset for flat-topped */
    }
    
    SylvesVector3 pos;
    pos.x = 0.5 * cos(angle);
    pos.y = 0.5 * sin(angle);
    pos.z = is_top ? 0.5 : -0.5;
    
    return pos;
}

static const char* hex_prism_name(const SylvesCellType* ct) {
    HexPrismCellData* data = (HexPrismCellData*)ct->data;
    return data->flat_topped ? "FlatToppedHexPrism" : "PointyToppedHexPrism";
}

static void hex_prism_destroy(SylvesCellType* ct) {
    /* Static instances, nothing to free */
}
