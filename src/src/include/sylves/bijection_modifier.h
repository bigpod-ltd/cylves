/**
 * @file bijection_modifier.h
 * @brief Bijection modifier for remapping cell coordinates
 *
 * The BijectModifier allows for arbitrary coordinate transformations
 * using bijective (one-to-one) functions and their inverses.
 *
 * This modifier supports:
 * - Forward mapping: transforming base grid coordinates to new coordinates
 * - Backward mapping: inverse transformation from new coordinates to base grid
 * - Injection of arbitrary coordinate mapping functions
 * - Automatic handling of cell conversion and enumeration
 *
 * Example usage:
 * @code
 * // Coordinate mapping function
 * void map_forward(SylvesCell in, SylvesCell* out) {
 *     out->x = in.y;
 *     out->y = -in.x;
 * }
 *
 * // Inverse mapping function
 * void map_backward(SylvesCell in, SylvesCell* out) {
 *     out->x = -in.y;
 *     out->y = in.x;
 * }
 *
 * // Create a bijection modifier with custom mapping functions
 * SylvesGrid* bijection = sylves_bijection_modifier_create(grid, map_forward, map_backward);
 *
 * @endcode
 */

#ifndef SYLVES_BIJECTION_MODIFIER_H
#define SYLVES_BIJECTION_MODIFIER_H

#include "grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function pointer type for cell coordinate mapping
 *
 * This function maps one grid cell coordinate to another, used for bijection.
 */
typedef void (*SylvesCellMapFunc)(SylvesCell input, SylvesCell* output);

/**
 * @brief Creates a bijection modifier that transforms coordinates
 *
 * The bijection modifier applies bijective mappings to cell coordinates
 * while preserving the structure and connectivity of the base grid.
 *
 * @param base_grid The underlying grid to map (must not be NULL)
 * @param forward The forward mapping function (must not be NULL)
 * @param backward The inverse mapping function (must not be NULL)
 * @return New bijection modifier grid, or NULL on error
 *
 * @note The forward and backward functions must be inverses
 * @note The created grid takes ownership of the base grid upon success
 */
SylvesGrid* sylves_bijection_modifier_create(
    SylvesGrid* base_grid,
    SylvesCellMapFunc forward,
    SylvesCellMapFunc backward);

/**
 * @brief Maps a cell coordinate using the forward mapping
 *
 * This function applies the forward mapping to a cell and returns its mapped position.
 *
 * @param grid The bijection modifier grid
 * @param cell The input cell coordinate
 * @param mapped Output for the mapped cell coordinate
 * @return true if mapping succeeded, false on error
 */
bool sylves_bijection_modifier_map_forward(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* mapped);

/**
 * @brief Maps a cell coordinate using the backward mapping
 *
 * This function applies the backward mapping to convert a mapped cell back
 * to its original position in the base grid.
 *
 * @param grid The bijection modifier grid
 * @param cell The mapped cell coordinate (in modified grid)
 * @param original Output for the original cell coordinate (in base grid)
 * @return true if mapping succeeded, false on error
 */
bool sylves_bijection_modifier_map_backward(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* original);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_BIJECTION_MODIFIER_H */
