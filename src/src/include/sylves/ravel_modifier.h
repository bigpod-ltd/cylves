/**
 * @file ravel_modifier.h
 * @brief Ravel modifier for flattening multi-dimensional grids
 * 
 * The ravel modifier flattens multi-dimensional grids into one-dimensional
 * linear sequences, similar to row-major or column-major array ordering.
 * This is useful for serialization, linear indexing, and space-filling curves.
 * 
 * Ravel ordering modes:
 * - Row-major: rightmost index varies fastest (C-style arrays)
 * - Column-major: leftmost index varies fastest (Fortran-style arrays)
 * - Morton order: Z-order curve for spatial locality
 * - Hilbert order: Hilbert curve for optimal spatial locality
 * 
 * Key features:
 * - Bijective mapping between multi-dimensional and linear indices
 * - Preserves spatial locality (especially with curve orders)
 * - Efficient forward and inverse transformations
 * - Support for different ordering schemes
 * 
 * Example usage:
 * @code
 * // Create a 10x10 2D grid
 * SylvesGrid* grid2d = sylves_square_grid_create_bounded(1.0, bounds);
 * 
 * // Flatten to 1D using row-major order
 * SylvesGrid* grid1d = sylves_ravel_modifier_create(
 *     grid2d, 
 *     SYLVES_RAVEL_ROW_MAJOR
 * );
 * 
 * // Cell (3,4) in 2D becomes cell 34 in 1D (for 10x10 grid)
 * SylvesCell cell2d = sylves_cell_create_2d(3, 4);
 * int index = sylves_ravel_modifier_get_index(grid1d, cell2d);
 * // index == 34
 * @endcode
 */

#ifndef SYLVES_RAVEL_MODIFIER_H
#define SYLVES_RAVEL_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ravel ordering modes
 */
typedef enum {
    /** Row-major order (rightmost index varies fastest) */
    SYLVES_RAVEL_ROW_MAJOR,
    
    /** Column-major order (leftmost index varies fastest) */
    SYLVES_RAVEL_COLUMN_MAJOR,
    
    /** Morton order (Z-order curve) */
    SYLVES_RAVEL_MORTON,
    
    /** Hilbert order (Hilbert curve) */
    SYLVES_RAVEL_HILBERT
} SylvesRavelOrder;

/**
 * @brief Creates a ravel modifier that flattens a grid to 1D
 * 
 * The ravel modifier creates a one-dimensional view of a multi-dimensional
 * grid by assigning each cell a unique linear index.
 * 
 * @param base_grid The multi-dimensional grid to flatten (must be bounded)
 * @param order The ordering scheme to use for flattening
 * @return New ravel modifier grid, or NULL on error
 * 
 * @note The base grid must be bounded (finite cell count)
 * @note The created grid takes ownership of the base grid
 */
SylvesGrid* sylves_ravel_modifier_create(
    SylvesGrid* base_grid,
    SylvesRavelOrder order);

/**
 * @brief Gets the linear index for a multi-dimensional cell
 * 
 * Converts a cell from the base grid's coordinate system to its
 * corresponding linear index in the flattened view.
 * 
 * @param grid The ravel modifier grid
 * @param cell The multi-dimensional cell
 * @return Linear index (0-based), or -1 on error
 */
int sylves_ravel_modifier_get_index(
    const SylvesGrid* grid,
    SylvesCell cell);

/**
 * @brief Gets the multi-dimensional cell for a linear index
 * 
 * Converts a linear index back to the corresponding cell in the
 * base grid's multi-dimensional coordinate system.
 * 
 * @param grid The ravel modifier grid
 * @param index The linear index (0-based)
 * @param cell Output for the multi-dimensional cell
 * @return true if successful, false if index out of bounds
 */
bool sylves_ravel_modifier_get_cell(
    const SylvesGrid* grid,
    int index,
    SylvesCell* cell);

/**
 * @brief Gets the ordering mode used by this modifier
 * 
 * @param grid The ravel modifier grid
 * @return The ravel order, or -1 if not a ravel modifier
 */
SylvesRavelOrder sylves_ravel_modifier_get_order(
    const SylvesGrid* grid);

/**
 * @brief Gets the total number of cells in the flattened grid
 * 
 * @param grid The ravel modifier grid
 * @return Total cell count, or -1 on error
 */
int sylves_ravel_modifier_get_count(const SylvesGrid* grid);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_RAVEL_MODIFIER_H */
