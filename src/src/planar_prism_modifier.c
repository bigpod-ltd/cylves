/**
 * @file planar_prism_modifier.c
 * @brief Implementation of the PlanarPrism modifier for extruding 2D grids to 3D
 * 
 * This file provides the implementation for the PlanarPrism Modifier, which
 * extrudes any 2D planar grid into 3D by creating prism cells.
 */

#include "sylves/planar_prism_modifier.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/cell.h"
#include "sylves/vector.h"
#include "sylves/connection.h"
#include "grid_internal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// Direction constants for 3D prism cells
// These are specific to the planar prism modifier
#define SYLVES_DIR_UP    4
#define SYLVES_DIR_DOWN  5

struct SylvesPlanarPrismModifier {
    SylvesGrid base;
    SylvesGrid* planar_grid;
    int num_layers;
    double* layer_heights;
    double total_height;
};

SylvesGrid*
sylves_planar_prism_modifier_create(
    SylvesGrid* base_grid,
    int num_layers,
    double layer_height
) {
    if (!base_grid || num_layers <= 0 || layer_height <= 0) {
        return NULL;
    }
    
    // Verify grid is 2D and planar
    if (!sylves_grid_is_2d(base_grid) || !sylves_grid_is_planar(base_grid)) {
        // PlanarPrism modifier requires a 2D planar grid
        return NULL;
    }
    
    struct SylvesPlanarPrismModifier* grid = (struct SylvesPlanarPrismModifier*)sylves_alloc(
        sizeof(struct SylvesPlanarPrismModifier));
    
    grid->planar_grid = base_grid;
    grid->num_layers = num_layers;
    
    // Create uniform layer heights
    grid->layer_heights = (double*)sylves_alloc(num_layers * sizeof(double));
    for (int i = 0; i < num_layers; i++) {
        grid->layer_heights[i] = layer_height;
    }
    grid->total_height = layer_height * num_layers;
    
    return (SylvesGrid*)grid;
}

SylvesGrid*
sylves_planar_prism_modifier_create_variable(
    SylvesGrid* base_grid,
    int num_layers,
    const double* layer_heights
) {
    if (!base_grid || num_layers <= 0 || !layer_heights) {
        return NULL;
    }
    
    // Verify grid is 2D and planar
    if (!sylves_grid_is_2d(base_grid) || !sylves_grid_is_planar(base_grid)) {
        // PlanarPrism modifier requires a 2D planar grid
        return NULL;
    }
    
    struct SylvesPlanarPrismModifier* grid = (struct SylvesPlanarPrismModifier*)sylves_alloc(
        sizeof(struct SylvesPlanarPrismModifier));
    
    grid->planar_grid = base_grid;
    grid->num_layers = num_layers;
    
    // Copy layer heights
    grid->layer_heights = (double*)sylves_alloc(num_layers * sizeof(double));
    grid->total_height = 0;
    for (int i = 0; i < num_layers; i++) {
        grid->layer_heights[i] = layer_heights[i];
        grid->total_height += layer_heights[i];
    }
    
    return (SylvesGrid*)grid;
}

static void
sylves_planar_prism_modifier_destroy(SylvesGrid* grid) {
    struct SylvesPlanarPrismModifier* self = (struct SylvesPlanarPrismModifier*)grid;
    sylves_free(self->layer_heights);
    sylves_grid_destroy(self->planar_grid);
    sylves_free(self);
}

static bool
sylves_planar_prism_modifier_is_3d(const SylvesGrid* grid) {
    return true;
}

static bool
sylves_planar_prism_modifier_is_2d(const SylvesGrid* grid) {
    return false;
}

static int
sylves_planar_prism_modifier_get_coordinate_dimension(const SylvesGrid* grid) {
    return 3;
}

static bool
sylves_planar_prism_modifier_is_cell_in_grid(
    const SylvesGrid* grid,
    SylvesCell cell
) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    
    // Check layer bounds
    if (cell.z < 0 || cell.z >= self->num_layers) {
        return false;
    }
    
    // Check 2D cell validity
    SylvesCell cell2d = sylves_cell_create_2d(cell.x, cell.y);
    return sylves_grid_is_cell_in_grid(self->planar_grid, cell2d);
}

static SylvesVector3
sylves_planar_prism_modifier_get_cell_center(
    const SylvesGrid* grid,
    SylvesCell cell
) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    
    // Get 2D center
    SylvesCell cell2d = sylves_cell_create_2d(cell.x, cell.y);
    SylvesVector3 center2d = sylves_grid_get_cell_center(self->planar_grid, cell2d);
    
    // Calculate layer height offset
    double z_offset = 0;
    for (int i = 0; i < cell.z; i++) {
        z_offset += self->layer_heights[i];
    }
    z_offset += self->layer_heights[cell.z] / 2.0;
    
    return sylves_vector3_create(center2d.x, center2d.y, z_offset);
}

static bool
sylves_planar_prism_modifier_try_move(
    const SylvesGrid* grid,
    SylvesCell from,
    SylvesCellDir dir,
    SylvesCell* to,
    SylvesCellDir* inverse_dir,
    SylvesConnection* connection
) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    
    // Handle vertical movement
    if (dir == SYLVES_DIR_UP) {
        if (from.z < self->num_layers - 1) {
            *to = sylves_cell_create(from.x, from.y, from.z + 1);
            if (inverse_dir) *inverse_dir = SYLVES_DIR_DOWN;
            if (connection) *connection = sylves_connection_identity();
            return true;
        }
        return false;
    } else if (dir == SYLVES_DIR_DOWN) {
        if (from.z > 0) {
            *to = sylves_cell_create(from.x, from.y, from.z - 1);
            if (inverse_dir) *inverse_dir = SYLVES_DIR_UP;
            if (connection) *connection = sylves_connection_identity();
            return true;
        }
        return false;
    }
    
    // Handle horizontal movement using base grid
    SylvesCell from2d = sylves_cell_create_2d(from.x, from.y);
    SylvesCell to2d;
    
    if (sylves_grid_try_move(self->planar_grid, from2d, dir, &to2d, inverse_dir, connection)) {
        *to = sylves_cell_create(to2d.x, to2d.y, from.z);
        return true;
    }
    
    return false;
}

int
sylves_planar_prism_modifier_get_layer_count(const SylvesGrid* grid) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    return self->num_layers;
}

double
sylves_planar_prism_modifier_get_layer_height(
    const SylvesGrid* grid,
    int layer
) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    
    if (layer < 0 || layer >= self->num_layers) {
        return NAN;
    }
    
    return self->layer_heights[layer];
}

const SylvesGrid*
sylves_planar_prism_modifier_get_base_grid(const SylvesGrid* grid) {
    const struct SylvesPlanarPrismModifier* self = (const struct SylvesPlanarPrismModifier*)grid;
    return self->planar_grid;
}

bool
sylves_planar_prism_modifier_project_to_base(
    const SylvesGrid* grid,
    SylvesCell cell3d,
    SylvesCell* cell2d
) {
    if (!grid || !cell2d) {
        return false;
    }
    
    *cell2d = sylves_cell_create_2d(cell3d.x, cell3d.y);
    return true;
}

static void
sylves_planar_prism_modifier_init_vtable(SylvesGridVTable* vtable) {
    memset(vtable, 0, sizeof(SylvesGridVTable));
    vtable->destroy = sylves_planar_prism_modifier_destroy;
    vtable->is_2d = sylves_planar_prism_modifier_is_2d;
    vtable->is_3d = sylves_planar_prism_modifier_is_3d;
    vtable->get_coordinate_dimension = sylves_planar_prism_modifier_get_coordinate_dimension;
    vtable->is_cell_in_grid = sylves_planar_prism_modifier_is_cell_in_grid;
    vtable->get_cell_center = sylves_planar_prism_modifier_get_cell_center;
    vtable->try_move = sylves_planar_prism_modifier_try_move;
}

static SylvesGridVTable planar_prism_vtable = {0};

void
sylves_planar_prism_modifier_vtable_init(SylvesGridVTable* vtable) {
    if (!planar_prism_vtable.destroy) {
        sylves_planar_prism_modifier_init_vtable(&planar_prism_vtable);
    }
    *vtable = planar_prism_vtable;
}
