/**
 * @file cube_grid.h
 * @brief Cube grid - 3D rectangular grid
 */

#ifndef SYLVES_CUBE_GRID_H
#define SYLVES_CUBE_GRID_H

#include "types.h"
#include "grid.h"

/* Cube directions */
typedef enum {
    SYLVES_CUBE_DIR_RIGHT = 0,   /* +X */
    SYLVES_CUBE_DIR_LEFT = 1,    /* -X */
    SYLVES_CUBE_DIR_UP = 2,      /* +Y */
    SYLVES_CUBE_DIR_DOWN = 3,    /* -Y */
    SYLVES_CUBE_DIR_FORWARD = 4, /* +Z */
    SYLVES_CUBE_DIR_BACK = 5,    /* -Z */
    SYLVES_CUBE_DIR_COUNT = 6,
} SylvesCubeDir;

/* Cube corners */
typedef enum {
    SYLVES_CUBE_CORNER_BACK_DOWN_LEFT = 0,
    SYLVES_CUBE_CORNER_BACK_DOWN_RIGHT = 1,
    SYLVES_CUBE_CORNER_BACK_UP_LEFT = 2,
    SYLVES_CUBE_CORNER_BACK_UP_RIGHT = 3,
    SYLVES_CUBE_CORNER_FORWARD_DOWN_LEFT = 4,
    SYLVES_CUBE_CORNER_FORWARD_DOWN_RIGHT = 5,
    SYLVES_CUBE_CORNER_FORWARD_UP_LEFT = 6,
    SYLVES_CUBE_CORNER_FORWARD_UP_RIGHT = 7,
    SYLVES_CUBE_CORNER_COUNT = 8,
} SylvesCubeCorner;

/* Creation */
SylvesGrid* sylves_cube_grid_create(double cell_size);
SylvesGrid* sylves_cube_grid_create_anisotropic(double cell_size_x, double cell_size_y, double cell_size_z);
SylvesGrid* sylves_cube_grid_create_bounded(double cell_size, 
                                            int min_x, int min_y, int min_z,
                                            int max_x, int max_y, int max_z);
SylvesGrid* sylves_cube_grid_create_bounded_anisotropic(double cell_size_x, double cell_size_y, double cell_size_z,
                                                        int min_x, int min_y, int min_z,
                                                        int max_x, int max_y, int max_z);

#endif /* SYLVES_CUBE_GRID_H */
