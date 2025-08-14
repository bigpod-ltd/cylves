/**
 * @file prism_grid.h
 * @brief Prism grids - 3D extensions of 2D grids
 */

#ifndef SYLVES_PRISM_GRID_H
#define SYLVES_PRISM_GRID_H

#include "types.h"
#include "grid.h"

/* Hex prism grid - extends hexagonal grid to 3D */
SylvesGrid* sylves_hex_prism_grid_create(bool flat_topped, double cell_size, double layer_height);
SylvesGrid* sylves_hex_prism_grid_create_bounded(bool flat_topped, double cell_size, double layer_height,
                                                  int min_q, int min_r, int max_q, int max_r,
                                                  int min_layer, int max_layer);

/* Triangle prism grid - extends triangular grid to 3D */
SylvesGrid* sylves_triangle_prism_grid_create(double cell_size, double layer_height);
SylvesGrid* sylves_triangle_prism_grid_create_bounded(double cell_size, double layer_height,
                                                       int min_x, int min_y, int min_z,
                                                       int max_x, int max_y, int max_z,
                                                       int min_layer, int max_layer);

/* Square prism grid - extends square grid to 3D (essentially a cube grid) */
SylvesGrid* sylves_square_prism_grid_create(double cell_size, double layer_height);
SylvesGrid* sylves_square_prism_grid_create_bounded(double cell_size, double layer_height,
                                                     int min_x, int min_y, int max_x, int max_y,
                                                     int min_layer, int max_layer);

#endif /* SYLVES_PRISM_GRID_H */
