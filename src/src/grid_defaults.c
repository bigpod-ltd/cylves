/**
 * @file grid_defaults.c
 * @brief Default implementations and property helpers for grids (impl)
 */

#include "grid_defaults.h"
#include "sylves/grid.h"
#include "sylves/cell_type.h"
#include "sylves/vector.h"

bool sylves_grid_default_is_2d(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_2d) return grid->vtable->is_2d(grid);
    /* Fallback: 2D if coordinate dimension is 2 */
    int dim = sylves_grid_get_coordinate_dimension(grid);
    return dim == 2;
}

bool sylves_grid_default_is_3d(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_3d) return grid->vtable->is_3d(grid);
    int dim = sylves_grid_get_coordinate_dimension(grid);
    return dim == 3;
}

bool sylves_grid_default_is_planar(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_planar) return grid->vtable->is_planar(grid);
    return sylves_grid_default_is_2d(grid);
}

bool sylves_grid_default_is_repeating(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_repeating) return grid->vtable->is_repeating(grid);
    switch (sylves_grid_get_type(grid)) {
        case SYLVES_GRID_TYPE_SQUARE:
        case SYLVES_GRID_TYPE_HEX:
        case SYLVES_GRID_TYPE_TRIANGLE:
        case SYLVES_GRID_TYPE_CUBE:
            return true;
        default:
            return false;
    }
}

bool sylves_grid_default_is_orientable(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_orientable) return grid->vtable->is_orientable(grid);
    return true; /* sensible default */
}

bool sylves_grid_default_is_finite(const SylvesGrid* grid) {
    if (!grid) return false;
    if (grid->vtable && grid->vtable->is_finite) return grid->vtable->is_finite(grid);
    return sylves_grid_get_bound(grid) != NULL;
}

int sylves_grid_default_coordinate_dimension(const SylvesGrid* grid) {
    if (!grid) return 0;
    if (grid->vtable && grid->vtable->get_coordinate_dimension) return grid->vtable->get_coordinate_dimension(grid);
    if (sylves_grid_default_is_3d(grid)) return 3;
    if (sylves_grid_default_is_2d(grid)) return 2;
    return 0;
}

