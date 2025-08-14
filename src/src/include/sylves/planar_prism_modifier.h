/**
 * @file planar_prism_modifier.h
 * @brief Planar prism modifier for extruding 2D grids into 3D
 * 
 * The planar prism modifier extrudes any 2D planar grid into 3D by creating
 * prism cells. Unlike the specific prism grids (HexPrismGrid, etc.), this
 * modifier works with any 2D grid type.
 * 
 * Key features:
 * - Works with any 2D planar grid (square, hex, triangle, mesh, etc.)
 * - Creates prism cells with customizable height
 * - Supports multiple layers with different heights
 * - Preserves the 2D grid topology in each layer
 * 
 * Cell addressing:
 * - Cells use 3D coordinates: (x, y, layer)
 * - The x,y coordinates match the base 2D grid
 * - The layer coordinate identifies the vertical level
 * 
 * Example usage:
 * @code
 * // Create any 2D grid
 * SylvesGrid* grid2d = sylves_hex_grid_create_bounded(
 *     SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0, bounds
 * );
 * 
 * // Extrude to 3D with 5 layers of height 2.0 each
 * SylvesGrid* grid3d = sylves_planar_prism_modifier_create(
 *     grid2d, 5, 2.0
 * );
 * 
 * // Access cells using 3D coordinates
 * SylvesCell cell = sylves_cell_create_3d(3, 4, 2); // layer 2
 * @endcode
 */

#ifndef SYLVES_PLANAR_PRISM_MODIFIER_H
#define SYLVES_PLANAR_PRISM_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a planar prism modifier with uniform layer heights
 * 
 * Extrudes a 2D planar grid into 3D by creating multiple layers of
 * prism cells, all with the same height.
 * 
 * @param base_grid The 2D planar grid to extrude (must be planar)
 * @param num_layers Number of vertical layers to create
 * @param layer_height Height of each layer
 * @return New planar prism modifier grid, or NULL on error
 * 
 * @note The base grid must be 2D and planar
 * @note The created grid takes ownership of the base grid
 * @note Total height is num_layers * layer_height
 */
SylvesGrid* sylves_planar_prism_modifier_create(
    SylvesGrid* base_grid,
    int num_layers,
    double layer_height);

/**
 * @brief Creates a planar prism modifier with variable layer heights
 * 
 * Extrudes a 2D planar grid into 3D with custom height for each layer,
 * allowing for non-uniform vertical spacing.
 * 
 * @param base_grid The 2D planar grid to extrude (must be planar)
 * @param num_layers Number of vertical layers to create
 * @param layer_heights Array of heights for each layer
 * @return New planar prism modifier grid, or NULL on error
 * 
 * @note layer_heights array must have num_layers elements
 * @note Heights are cumulative from bottom to top
 */
SylvesGrid* sylves_planar_prism_modifier_create_variable(
    SylvesGrid* base_grid,
    int num_layers,
    const double* layer_heights);

/**
 * @brief Gets the number of layers in this prism modifier
 * 
 * @param grid The planar prism modifier grid
 * @return Number of layers, or -1 if not a planar prism modifier
 */
int sylves_planar_prism_modifier_get_layer_count(
    const SylvesGrid* grid);

/**
 * @brief Gets the height of a specific layer
 * 
 * @param grid The planar prism modifier grid
 * @param layer The layer index (0-based)
 * @return Height of the layer, or NaN on error
 */
double sylves_planar_prism_modifier_get_layer_height(
    const SylvesGrid* grid,
    int layer);

/**
 * @brief Gets the base 2D grid from this modifier
 * 
 * @param grid The planar prism modifier grid
 * @return The base 2D grid, or NULL if not a planar prism modifier
 * 
 * @note The returned grid is owned by the modifier, do not destroy
 */
const SylvesGrid* sylves_planar_prism_modifier_get_base_grid(
    const SylvesGrid* grid);

/**
 * @brief Projects a 3D cell to its 2D base cell
 * 
 * Removes the layer component to get the corresponding cell in the
 * base 2D grid.
 * 
 * @param grid The planar prism modifier grid
 * @param cell3d The 3D cell in the prism grid
 * @param cell2d Output for the 2D cell in the base grid
 * @return true if successful, false on error
 */
bool sylves_planar_prism_modifier_project_to_base(
    const SylvesGrid* grid,
    SylvesCell cell3d,
    SylvesCell* cell2d);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_PLANAR_PRISM_MODIFIER_H */
