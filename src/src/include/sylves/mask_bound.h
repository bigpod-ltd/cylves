/**
 * @file mask_bound.h
 * @brief MaskBound type for arbitrary cell sets
 */

#ifndef SYLVES_MASK_BOUND_H
#define SYLVES_MASK_BOUND_H

#include "types.h"
#include "bounds.h"
#include <stddef.h>

/* MaskBound - Bound type defined by a set of cells */

/**
 * Create a MaskBound from an array of cells
 * 
 * @param cells Array of cells to include in the bound
 * @param cell_count Number of cells
 * @return New mask bound or NULL on error
 */
SylvesBound* sylves_bound_create_mask(const SylvesCell* cells, size_t cell_count);

/**
 * Create a MaskBound from a base bound with a filter function
 * 
 * @param base Base bound to filter
 * @param filter Function that returns true if cell should be included
 * @param user_data User data passed to filter function
 * @return New mask bound or NULL on error
 */
SylvesBound* sylves_bound_create_mask_filtered(
    const SylvesBound* base,
    bool (*filter)(SylvesCell cell, void* user_data),
    void* user_data);

/**
 * Add cells to an existing MaskBound
 * 
 * @param bound MaskBound to modify
 * @param cells Array of cells to add
 * @param cell_count Number of cells to add
 * @return 0 on success, negative error code on failure
 */
int sylves_mask_bound_add_cells(SylvesBound* bound, const SylvesCell* cells, size_t cell_count);

/**
 * Remove cells from an existing MaskBound
 * 
 * @param bound MaskBound to modify
 * @param cells Array of cells to remove
 * @param cell_count Number of cells to remove
 * @return 0 on success, negative error code on failure
 */
int sylves_mask_bound_remove_cells(SylvesBound* bound, const SylvesCell* cells, size_t cell_count);

/**
 * Get the number of cells in a MaskBound
 * 
 * @param bound MaskBound to query
 * @return Number of cells, or negative error code
 */
int sylves_mask_bound_get_count(const SylvesBound* bound);

/**
 * Clear all cells from a MaskBound
 * 
 * @param bound MaskBound to clear
 * @return 0 on success, negative error code on failure
 */
int sylves_mask_bound_clear(SylvesBound* bound);

#endif /* SYLVES_MASK_BOUND_H */
