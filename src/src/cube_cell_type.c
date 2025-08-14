/**
 * @file cube_cell_type.c
 * @brief Cube cell type implementation
 */

#include "sylves/cube_cell_type.h"
#include "internal/cell_type_internal.h"
#include "sylves/vector.h"
#include <stdlib.h>

/* Forward declarations */
static int cube_get_dimension(const SylvesCellType* ct);
static int cube_get_dir_count(const SylvesCellType* ct);
static int cube_get_corner_count(const SylvesCellType* ct);
static SylvesVector3 cube_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c);
static const char* cube_name(const SylvesCellType* ct);
static void cube_destroy(SylvesCellType* ct);

/* VTable for cube cell type */
static const SylvesCellTypeVTable cube_vtable = {
    .get_dimension = cube_get_dimension,
    .get_dir_count = cube_get_dir_count,
    .get_corner_count = cube_get_corner_count,
    .get_corner_pos = cube_get_corner_pos,
    .name = cube_name,
    .destroy = cube_destroy
};

/* Static instance */
static SylvesCellType cube_instance = {
    .vtable = &cube_vtable,
    .data = NULL
};

const SylvesCellType* sylves_cube_cell_type_get(void) {
    return &cube_instance;
}

/* Implementation functions */
static int cube_get_dimension(const SylvesCellType* ct) {
    return 3; /* Cubes are 3D */
}

static int cube_get_dir_count(const SylvesCellType* ct) {
    return 6; /* 6 faces */
}

static int cube_get_corner_count(const SylvesCellType* ct) {
    return 8; /* 8 corners */
}

static SylvesVector3 cube_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c) {
    /* Get corner position based on corner enum */
    SylvesVector3 pos;
    
    switch (c) {
        case SYLVES_CUBE_CORNER_BACK_DOWN_LEFT:
            pos.x = -0.5; pos.y = -0.5; pos.z = -0.5;
            break;
        case SYLVES_CUBE_CORNER_BACK_DOWN_RIGHT:
            pos.x = 0.5; pos.y = -0.5; pos.z = -0.5;
            break;
        case SYLVES_CUBE_CORNER_BACK_UP_LEFT:
            pos.x = -0.5; pos.y = 0.5; pos.z = -0.5;
            break;
        case SYLVES_CUBE_CORNER_BACK_UP_RIGHT:
            pos.x = 0.5; pos.y = 0.5; pos.z = -0.5;
            break;
        case SYLVES_CUBE_CORNER_FORWARD_DOWN_LEFT:
            pos.x = -0.5; pos.y = -0.5; pos.z = 0.5;
            break;
        case SYLVES_CUBE_CORNER_FORWARD_DOWN_RIGHT:
            pos.x = 0.5; pos.y = -0.5; pos.z = 0.5;
            break;
        case SYLVES_CUBE_CORNER_FORWARD_UP_LEFT:
            pos.x = -0.5; pos.y = 0.5; pos.z = 0.5;
            break;
        case SYLVES_CUBE_CORNER_FORWARD_UP_RIGHT:
            pos.x = 0.5; pos.y = 0.5; pos.z = 0.5;
            break;
        default:
            pos.x = pos.y = pos.z = 0.0;
            break;
    }
    
    return pos;
}

static const char* cube_name(const SylvesCellType* ct) {
    return "Cube";
}

static void cube_destroy(SylvesCellType* ct) {
    /* Static instance, nothing to free */
}
