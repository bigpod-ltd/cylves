/**
 * @file cell.h
 * @brief Cell operations and utilities
 */

#ifndef SYLVES_CELL_H
#define SYLVES_CELL_H

#include "types.h"


/**
 * @brief Create a cell with given coordinates
 */
static inline SylvesCell sylves_cell_create(int x, int y, int z) {
    SylvesCell cell = {x, y, z};
    return cell;
}

/**
 * @brief Create a 2D cell (z = 0)
 */
static inline SylvesCell sylves_cell_create_2d(int x, int y) {
    return sylves_cell_create(x, y, 0);
}

/**
 * @brief Check if two cells are equal
 */
static inline bool sylves_cell_equals(SylvesCell a, SylvesCell b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/**
 * @brief Add offset to cell
 */
static inline SylvesCell sylves_cell_add(SylvesCell cell, SylvesVector3Int offset) {
    return sylves_cell_create(cell.x + offset.x, cell.y + offset.y, cell.z + offset.z);
}

/**
 * @brief Subtract cells to get offset
 */
static inline SylvesVector3Int sylves_cell_subtract(SylvesCell a, SylvesCell b) {
    SylvesVector3Int result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}


#endif /* SYLVES_CELL_H */
