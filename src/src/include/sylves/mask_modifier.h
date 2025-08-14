#ifndef SYLVES_MASK_MODIFIER_H
#define SYLVES_MASK_MODIFIER_H

#include "sylves/grid.h"
#include "sylves/grid_modifier.h"
#include <stdbool.h>

/**
 * @brief Function type for testing if a cell is in the mask
 */
typedef bool (*SylvesMaskContainsFunc)(SylvesCell cell, void* user_data);

/**
 * @brief Create a mask modifier that filters a grid to a subset of cells
 * 
 * The mask modifier restricts the grid to only cells that pass the contains test.
 * Filtered cells will not be returned by GetCells, TryMove, etc.
 * 
 * @param underlying The grid to filter
 * @param contains_func Function to test if a cell is in the mask
 * @param user_data User data passed to contains_func
 * @param cells Optional array of all cells in the mask (for finite masks)
 * @param cell_count Number of cells in the cells array (0 if not provided)
 * @return New masked grid, or NULL on error
 */
SylvesGrid* sylves_mask_modifier_create(
    SylvesGrid* underlying,
    SylvesMaskContainsFunc contains_func,
    void* user_data,
    const SylvesCell* cells,
    size_t cell_count);

/**
 * @brief Create a mask modifier from a set of cells
 * 
 * Convenience function that creates a mask from an explicit set of cells.
 * 
 * @param underlying The grid to filter
 * @param cells Array of cells to include in the mask
 * @param cell_count Number of cells in the array
 * @return New masked grid, or NULL on error
 */
SylvesGrid* sylves_mask_modifier_create_from_cells(
    SylvesGrid* underlying,
    const SylvesCell* cells,
    size_t cell_count);

#endif // SYLVES_MASK_MODIFIER_H
