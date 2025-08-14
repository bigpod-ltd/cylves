/**
 * @file square_grid_internal.h
 * @brief Internal helpers for square grid used by generic grid API
 */
#ifndef SQUARE_GRID_INTERNAL_H
#define SQUARE_GRID_INTERNAL_H

#include "sylves/types.h"


/* Enumerate cells in a bounded square grid; returns count or negative error. */
int sylves_square_grid_enumerate_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells);

/* Get total cell count for a bounded square grid; returns count or negative error. */
int sylves_square_grid_cell_count(const SylvesGrid* grid);

/* Get cells overlapping an AABB for square grids; returns number written. */
int sylves_square_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                         SylvesCell* cells, size_t max_cells);

/* Create a new grid that is this grid bounded by the given bound (rectangle expected). */
SylvesGrid* sylves_square_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound);

/* Create a new unbounded clone of this grid. */
SylvesGrid* sylves_square_grid_unbounded_clone(const SylvesGrid* grid);


#endif /* SQUARE_GRID_INTERNAL_H */

