/**
 * @file square_grid.h
 * @brief Square grid implementation
 */

#ifndef SYLVES_SQUARE_GRID_H
#define SYLVES_SQUARE_GRID_H

#include "types.h"
#include "grid.h"


/**
 * @brief Square cell directions
 */
typedef enum {
    SYLVES_SQUARE_DIR_RIGHT = 0,
    SYLVES_SQUARE_DIR_UP = 1,
    SYLVES_SQUARE_DIR_LEFT = 2,
    SYLVES_SQUARE_DIR_DOWN = 3,
    SYLVES_SQUARE_DIR_COUNT = 4
} SylvesSquareDir;

/**
 * @brief Square cell corners
 */
typedef enum {
    SYLVES_SQUARE_CORNER_BOTTOM_RIGHT = 0,
    SYLVES_SQUARE_CORNER_TOP_RIGHT = 1,
    SYLVES_SQUARE_CORNER_TOP_LEFT = 2,
    SYLVES_SQUARE_CORNER_BOTTOM_LEFT = 3,
    SYLVES_SQUARE_CORNER_COUNT = 4
} SylvesSquareCorner;

/**
 * @brief Create a square grid with specified cell size
 * @param cell_size Size of each square cell
 * @return New square grid, or NULL on error
 */
SylvesGrid* sylves_square_grid_create(double cell_size);

/**
 * @brief Create a bounded square grid
 * @param cell_size Size of each square cell
 * @param min_x Minimum X coordinate
 * @param min_y Minimum Y coordinate
 * @param max_x Maximum X coordinate
 * @param max_y Maximum Y coordinate
 * @return New bounded square grid, or NULL on error
 */
SylvesGrid* sylves_square_grid_create_bounded(double cell_size, 
                                              int min_x, int min_y,
                                              int max_x, int max_y);


#endif /* SYLVES_SQUARE_GRID_H */
