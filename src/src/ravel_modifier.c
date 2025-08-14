/**
 * @file ravel_modifier.c
 * @brief Implementation of the Ravel modifier for flattening multi-dimensional grids
 * 
 * This file provides the implementation for the Ravel Modifier, which converts
 * multi-dimensional grids into one-dimensional linear sequences.
 */

#include "sylves/ravel_modifier.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/cell.h"
#include "grid_internal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

struct SylvesRavelModifier {
    SylvesGrid base;
    SylvesGrid* raveled;
    SylvesRavelOrder order;
    int total_cells;
    int* dimensions;
    int num_dimensions;
};

/**
 * @brief Morton encoding (Z-order) interleave bits
 */
static int
morton_encode_2d(int x, int y) {
    int result = 0;
    for (int i = 0; i < 16; i++) {
        result |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
    }
    return result;
}

/**
 * @brief Morton decoding (Z-order) deinterleave bits
 */
static void
morton_decode_2d(int code, int* x, int* y) {
    *x = *y = 0;
    for (int i = 0; i < 16; i++) {
        *x |= ((code >> (2 * i)) & 1) << i;
        *y |= ((code >> (2 * i + 1)) & 1) << i;
    }
}

/**
 * @brief Row-major indexing (C-style)
 */
static int
row_major_index(const int* coords, const int* dims, int ndims) {
    int index = 0;
    int stride = 1;
    for (int i = ndims - 1; i >= 0; i--) {
        index += coords[i] * stride;
        stride *= dims[i];
    }
    return index;
}

/**
 * @brief Column-major indexing (Fortran-style)
 */
static int
column_major_index(const int* coords, const int* dims, int ndims) {
    int index = 0;
    int stride = 1;
    for (int i = 0; i < ndims; i++) {
        index += coords[i] * stride;
        stride *= dims[i];
    }
    return index;
}

/**
 * @brief Inverse row-major indexing
 */
static void
row_major_coords(int index, int* coords, const int* dims, int ndims) {
    for (int i = ndims - 1; i >= 0; i--) {
        coords[i] = index % dims[i];
        index /= dims[i];
    }
}

/**
 * @brief Inverse column-major indexing
 */
static void
column_major_coords(int index, int* coords, const int* dims, int ndims) {
    for (int i = 0; i < ndims; i++) {
        coords[i] = index % dims[i];
        index /= dims[i];
    }
}

SylvesGrid*
sylves_ravel_modifier_create(
    SylvesGrid* base_grid,
    SylvesRavelOrder order
) {
    if (!base_grid) return NULL;

    // Ensure grid is bounded
    if (!sylves_grid_is_finite(base_grid)) {
        // Ravel modifier requires a bounded grid
        return NULL;
    }

    struct SylvesRavelModifier* grid = (struct SylvesRavelModifier*)sylves_alloc(sizeof(struct SylvesRavelModifier));
    grid->raveled = base_grid;
    grid->order = order;
    
    // Get grid dimensions - simplified for now
    grid->num_dimensions = sylves_grid_get_coordinate_dimension(base_grid);
    grid->dimensions = (int*)sylves_alloc(grid->num_dimensions * sizeof(int));
    
    // Calculate total cells
    grid->total_cells = sylves_grid_get_cell_count(base_grid);

    return (SylvesGrid*)grid;
}

static void
sylves_ravel_modifier_destroy(SylvesGrid* grid) {
    struct SylvesRavelModifier* self = (struct SylvesRavelModifier*)grid;
    sylves_free(self->dimensions);
    sylves_grid_destroy(self->raveled);
    sylves_free(self);
}

int
sylves_ravel_modifier_get_index(
    const SylvesGrid* grid,
    SylvesCell cell
) {
    const struct SylvesRavelModifier* self = (const struct SylvesRavelModifier*)grid;
    
    // Extract coordinates from cell
    int coords[4] = {cell.x, cell.y, cell.z, 0};
    
    switch (self->order) {
        case SYLVES_RAVEL_ROW_MAJOR:
            return row_major_index(coords, self->dimensions, self->num_dimensions);
            
        case SYLVES_RAVEL_COLUMN_MAJOR:
            return column_major_index(coords, self->dimensions, self->num_dimensions);
            
        case SYLVES_RAVEL_MORTON:
            if (self->num_dimensions == 2) {
                return morton_encode_2d(cell.x, cell.y);
            }
            // Fall through to row-major for 3D+ for now
            
        case SYLVES_RAVEL_HILBERT:
            // Hilbert curve not implemented yet, use row-major
            return row_major_index(coords, self->dimensions, self->num_dimensions);
            
        default:
            return -1;
    }
}

bool
sylves_ravel_modifier_get_cell(
    const SylvesGrid* grid,
    int index,
    SylvesCell* cell
) {
    const struct SylvesRavelModifier* self = (const struct SylvesRavelModifier*)grid;
    
    if (!cell || index < 0 || index >= self->total_cells) {
        return false;
    }
    
    int coords[4] = {0, 0, 0, 0};
    
    switch (self->order) {
        case SYLVES_RAVEL_ROW_MAJOR:
            row_major_coords(index, coords, self->dimensions, self->num_dimensions);
            break;
            
        case SYLVES_RAVEL_COLUMN_MAJOR:
            column_major_coords(index, coords, self->dimensions, self->num_dimensions);
            break;
            
        case SYLVES_RAVEL_MORTON:
            if (self->num_dimensions == 2) {
                morton_decode_2d(index, &coords[0], &coords[1]);
            } else {
                row_major_coords(index, coords, self->dimensions, self->num_dimensions);
            }
            break;
            
        case SYLVES_RAVEL_HILBERT:
            // Hilbert curve not implemented yet, use row-major
            row_major_coords(index, coords, self->dimensions, self->num_dimensions);
            break;
            
        default:
            return false;
    }
    
    *cell = sylves_cell_create(coords[0], coords[1], coords[2]);
    return true;
}

SylvesRavelOrder
sylves_ravel_modifier_get_order(const SylvesGrid* grid) {
    const struct SylvesRavelModifier* self = (const struct SylvesRavelModifier*)grid;
    return self->order;
}

int
sylves_ravel_modifier_get_count(const SylvesGrid* grid) {
    const struct SylvesRavelModifier* self = (const struct SylvesRavelModifier*)grid;
    return self->total_cells;
}

static void
sylves_ravel_modifier_init_vtable(SylvesGridVTable* vtable) {
    memset(vtable, 0, sizeof(SylvesGridVTable));
    vtable->destroy = sylves_ravel_modifier_destroy;
}

static SylvesGridVTable ravel_vtable = {0};

void
sylves_ravel_modifier_vtable_init(SylvesGridVTable* vtable) {
    if (!ravel_vtable.destroy) {
        sylves_ravel_modifier_init_vtable(&ravel_vtable);
    }
    *vtable = ravel_vtable;
}
