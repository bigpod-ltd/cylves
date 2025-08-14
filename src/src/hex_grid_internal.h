/**
 * @file hex_grid_internal.h
 * @brief Internal helpers for hex grid used by generic grid API
 */
#ifndef HEX_GRID_INTERNAL_H
#define HEX_GRID_INTERNAL_H

#include "sylves/types.h"


/* Enumerate cells in a bounded hex grid; returns count or negative error. */
int sylves_hex_grid_enumerate_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells);

/* Get total cell count for a bounded hex grid; returns count or negative error. */
int sylves_hex_grid_cell_count(const SylvesGrid* grid);

/* Get cells overlapping an AABB for hex grids; returns number written (clamped to max). */
int sylves_hex_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                      SylvesCell* cells, size_t max_cells);

/* Create a new grid that is this grid bounded by the given bound (rectangle expected). */
SylvesGrid* sylves_hex_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound);

/* Create a new unbounded clone of this grid. */
SylvesGrid* sylves_hex_grid_unbounded_clone(const SylvesGrid* grid);


#endif /* HEX_GRID_INTERNAL_H */

