/**
 * @file wrap_modifier.h
 * @brief Wrap modifier for creating toroidal topologies from base grids
 * 
 * The wrap modifier creates toroidal topologies by connecting opposite edges
 * of bounded grids. This allows movement from one edge to wrap around to the
 * opposite edge, creating a topology without boundaries.
 * 
 * The wrap modifier supports:
 * - 1D wrapping (cylinder topology) - wrap in one direction only
 * - 2D wrapping (torus topology) - wrap in both horizontal and vertical directions
 * - 3D wrapping (hypertorus topology) - wrap in all three dimensions
 * 
 * Wrapping behavior:
 * - Cell coordinates are modulo-wrapped within the base grid bounds
 * - Movement across boundaries connects to opposite edges
 * - Spatial positions are continuous across wrapped boundaries
 * - Cell enumeration includes all cells exactly once
 * 
 * Example usage:
 * @code
 * // Create a 10x10 square grid
 * SylvesSquareBound* bound = sylves_square_bound_create_minmax(
 *     sylves_vector2i_create(0, 0),
 *     sylves_vector2i_create(10, 10)
 * );
 * SylvesGrid* square = sylves_square_grid_create_bounded(1.0, bound);
 * 
 * // Wrap in both directions to create a torus
 * SylvesGrid* torus = sylves_wrap_modifier_create(square, true, true, false);
 * 
 * // Movement from (9, 5) right wraps to (0, 5)
 * SylvesCell from = sylves_cell_create_2d(9, 5);
 * SylvesCell to;
 * SylvesConnection connection;
 * bool moved = sylves_grid_try_move(torus, from, SYLVES_SQUARE_DIR_RIGHT, &to, &connection);
 * // moved == true, to == (0, 5)
 * @endcode
 */

#ifndef SYLVES_WRAP_MODIFIER_H
#define SYLVES_WRAP_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a wrap modifier that adds toroidal topology to a bounded grid
 * 
 * The wrap modifier connects opposite edges of a bounded grid to create a
 * topology without boundaries. Movement and spatial queries wrap around
 * seamlessly.
 * 
 * @param base_grid The underlying bounded grid to wrap (must have finite bounds)
 * @param wrap_x Whether to wrap in the X direction
 * @param wrap_y Whether to wrap in the Y direction  
 * @param wrap_z Whether to wrap in the Z direction (for 3D grids)
 * @return New wrap modifier grid, or NULL on error
 * 
 * @note The base grid must be bounded (have finite cell count) for wrapping
 * @note The created grid takes ownership of the base grid
 * @note At least one wrap direction must be enabled
 */
SylvesGrid* sylves_wrap_modifier_create(
    SylvesGrid* base_grid,
    bool wrap_x,
    bool wrap_y,
    bool wrap_z);

/**
 * @brief Normalizes a cell coordinate to be within the wrapped bounds
 * 
 * This function takes a cell that may be outside the base grid bounds and
 * wraps its coordinates to the equivalent cell within bounds.
 * 
 * @param grid The wrap modifier grid
 * @param cell The cell to normalize (may be outside bounds)
 * @param normalized Output for the normalized cell within bounds
 * @return true if normalization succeeded, false on error
 * 
 * Example:
 * - Base grid bounds: (0,0) to (10,10)
 * - Cell (12, 5) normalizes to (2, 5)
 * - Cell (-3, 7) normalizes to (7, 7)
 */
bool sylves_wrap_modifier_normalize_cell(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* normalized);

/**
 * @brief Gets the wrapping dimensions for this modifier
 * 
 * @param grid The wrap modifier grid
 * @param wrap_x Output for whether X wrapping is enabled (can be NULL)
 * @param wrap_y Output for whether Y wrapping is enabled (can be NULL)
 * @param wrap_z Output for whether Z wrapping is enabled (can be NULL)
 * @return true if query succeeded, false if not a wrap modifier
 */
bool sylves_wrap_modifier_get_wrap_dimensions(
    const SylvesGrid* grid,
    bool* wrap_x,
    bool* wrap_y,
    bool* wrap_z);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_WRAP_MODIFIER_H */
