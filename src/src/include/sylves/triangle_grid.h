/**
 * @file triangle_grid.h
 * @brief triangle grid - to be implemented
 */

#ifndef SYLVES_TRIANGLE_GRID_H
#define SYLVES_TRIANGLE_GRID_H

#include "types.h"
#include "grid.h"


/* Triangle grid orientation, matching Sylves TriangleOrientation */
typedef enum {
    SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED = 0,
    SYLVES_TRIANGLE_ORIENTATION_FLAT_SIDES = 1,
} SylvesTriangleOrientation;

/* Creation */
SylvesGrid* sylves_triangle_grid_create(double cell_size, SylvesTriangleOrientation orientation);
SylvesGrid* sylves_triangle_grid_create_bounded(double cell_size, SylvesTriangleOrientation orientation,
                                                int min_x, int min_y, int min_z,
                                                int max_x, int max_y, int max_z);


#endif /* SYLVES_TRIANGLE_GRID_H */
