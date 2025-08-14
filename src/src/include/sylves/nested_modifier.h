/**
 * @file nested_modifier.h
 * @brief Nested modifier for creating hierarchical grids
 * 
 * The nested modifier creates hierarchical grids by replacing each cell of a
 * base grid with an entire child grid. This allows for multi-level grid
 * structures with different resolutions at each level.
 * 
 * Key features:
 * - Each cell of the base grid contains a complete child grid
 * - Cells are addressed using compound coordinates (base_cell, child_cell)
 * - Navigation seamlessly crosses boundaries between child grids
 * - Spatial queries work across hierarchy levels
 * 
 * Common use cases:
 * - Multi-resolution grids (coarse outer grid, fine inner grids)
 * - Hierarchical spatial indexing
 * - Adaptive mesh refinement patterns
 * - Fractal or recursive grid structures
 * 
 * Example usage:
 * @code
 * // Create a 3x3 base grid
 * SylvesGrid* base = sylves_square_grid_create_bounded(10.0, bounds);
 * 
 * // Create a 5x5 child grid template
 * SylvesGrid* child = sylves_square_grid_create_bounded(2.0, child_bounds);
 * 
 * // Create nested grid - each base cell contains a 5x5 grid
 * SylvesGrid* nested = sylves_nested_modifier_create(base, child);
 * 
 * // Address cells using compound coordinates
 * SylvesCell cell = sylves_nested_cell_create(
 *     sylves_cell_create_2d(1, 2),  // base cell
 *     sylves_cell_create_2d(3, 4)   // child cell
 * );
 * @endcode
 */

#ifndef SYLVES_NESTED_MODIFIER_H
#define SYLVES_NESTED_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a nested modifier for hierarchical grids
 * 
 * The nested modifier replaces each cell of the base grid with a copy of the
 * child grid, creating a two-level hierarchy. Cells are addressed using
 * compound coordinates.
 * 
 * @param base_grid The outer/coarse grid (must not be NULL)
 * @param child_grid Template for the inner/fine grids (must not be NULL)
 * @return New nested modifier grid, or NULL on error
 * 
 * @note Both grids must be bounded (finite cell count)
 * @note The created grid takes ownership of both input grids
 * @note Child grids are scaled to fit within their parent cells
 */
SylvesGrid* sylves_nested_modifier_create(
    SylvesGrid* base_grid,
    SylvesGrid* child_grid);

/**
 * @brief Creates a nested cell from base and child components
 * 
 * Nested cells use compound addressing with two components:
 * - Base cell: identifies which cell of the outer grid
 * - Child cell: identifies which cell within that base cell
 * 
 * @param base_cell The cell in the base/outer grid
 * @param child_cell The cell in the child/inner grid
 * @return Compound nested cell
 */
SylvesCell sylves_nested_cell_create(
    SylvesCell base_cell,
    SylvesCell child_cell);

/**
 * @brief Extracts the base cell component from a nested cell
 * 
 * @param nested_cell The compound nested cell
 * @return The base/outer grid cell component
 */
SylvesCell sylves_nested_cell_get_base(SylvesCell nested_cell);

/**
 * @brief Extracts the child cell component from a nested cell
 * 
 * @param nested_cell The compound nested cell
 * @return The child/inner grid cell component
 */
SylvesCell sylves_nested_cell_get_child(SylvesCell nested_cell);

/**
 * @brief Gets the depth of nesting for this modifier
 * 
 * @param grid The nested modifier grid
 * @return Number of hierarchy levels (2 for simple nesting), or 0 on error
 */
int sylves_nested_modifier_get_depth(const SylvesGrid* grid);

/**
 * @brief Gets the base grid from a nested modifier
 * 
 * @param grid The nested modifier grid
 * @return The base/outer grid, or NULL if not a nested modifier
 * 
 * @note The returned grid is owned by the modifier, do not destroy
 */
const SylvesGrid* sylves_nested_modifier_get_base_grid(
    const SylvesGrid* grid);

/**
 * @brief Gets the child grid template from a nested modifier
 * 
 * @param grid The nested modifier grid
 * @return The child grid template, or NULL if not a nested modifier
 * 
 * @note The returned grid is owned by the modifier, do not destroy
 */
const SylvesGrid* sylves_nested_modifier_get_child_grid(
    const SylvesGrid* grid);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_NESTED_MODIFIER_H */
