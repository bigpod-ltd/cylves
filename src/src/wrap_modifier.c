/**
 * @file wrap_modifier.c
 * @brief Wrap modifier implementation for toroidal topologies
 * 
 * This file implements the WrapModifier, an extension of a grid that wraps
 * around its borders to create toroidal connectivity, effectively removing
 * boundaries.
 */

#include "sylves/wrap_modifier.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/aabb.h"
#include "grid_internal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

struct SylvesWrapModifier {
    SylvesGrid base;
    SylvesGrid* wrapped;
    bool wrap_x;
    bool wrap_y;
    bool wrap_z;
    int min_x, max_x;
    int min_y, max_y;
    int min_z, max_z;
};

/* Forward declaration for internal use */
static bool wrap_normalize_cell(const struct SylvesWrapModifier* self, SylvesCell cell, SylvesCell* normalized);

static bool
sylves_wrap_modifier_try_move(
    const SylvesGrid* grid,
    SylvesCell from,
    SylvesCellDir dir,
    SylvesCell* to,
    SylvesCellDir* inverse_dir,
    SylvesConnection* connection
) {
    const struct SylvesWrapModifier* self = (const struct SylvesWrapModifier*)grid;
    
    /* Normalize the from cell first */
    SylvesCell normalized_from;
    if (!wrap_normalize_cell(self, from, &normalized_from)) {
        return false;
    }
    
    /* Try move in underlying grid */
    if (!sylves_grid_try_move(self->wrapped, normalized_from, dir, to, inverse_dir, connection)) {
        return false;
    }

    /* Normalize the result */
    return wrap_normalize_cell(self, *to, to);
}

static bool
wrap_normalize_cell(const struct SylvesWrapModifier* self, SylvesCell cell, SylvesCell* normalized) {
    *normalized = cell;
    
    /* Modulo wrap coordinates */
    if (self->wrap_x) {
        int range = self->max_x - self->min_x;
        normalized->x = self->min_x + ((cell.x - self->min_x) % range + range) % range;
    }
    if (self->wrap_y) {
        int range = self->max_y - self->min_y;
        normalized->y = self->min_y + ((cell.y - self->min_y) % range + range) % range;
    }
    if (self->wrap_z) {
        int range = self->max_z - self->min_z;
        normalized->z = self->min_z + ((cell.z - self->min_z) % range + range) % range;
    }
    
    return true;
}

SylvesGrid*
sylves_wrap_modifier_create(
    SylvesGrid* base_grid,
    bool wrap_x,
    bool wrap_y,
    bool wrap_z
) {
    if (!base_grid) return NULL;

    const SylvesBound* bounds = sylves_grid_get_bound(base_grid);
    if (!bounds) {
        sylves_grid_destroy(base_grid);
        return NULL;
    }

    struct SylvesWrapModifier* grid = (struct SylvesWrapModifier*)sylves_alloc(sizeof(struct SylvesWrapModifier));
    grid->wrapped = base_grid;
    grid->wrap_x = wrap_x;
    grid->wrap_y = wrap_y;
    grid->wrap_z = wrap_z;

    /* Initialize base grid structure */
    grid->base.vtable = NULL;  /* Will be set later */
    grid->base.type = SYLVES_GRID_TYPE_MODIFIER;
    grid->base.bound = bounds;
    grid->base.data = grid;
    return (SylvesGrid*)grid;
}

bool
sylves_wrap_modifier_get_wrap_dimensions(
    const SylvesGrid* grid,
    bool* wrap_x,
    bool* wrap_y,
    bool* wrap_z
) {
    if (!grid) return false;

    struct SylvesWrapModifier* self = (struct SylvesWrapModifier*)grid;
    if (wrap_x) *wrap_x = self->wrap_x;
    if (wrap_y) *wrap_y = self->wrap_y;
    if (wrap_z) *wrap_z = self->wrap_z;

    return true;
}

bool
sylves_wrap_modifier_normalize_cell(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* normalized
) {
    if (!grid || !normalized) return false;
    const struct SylvesWrapModifier* self = (const struct SylvesWrapModifier*)grid;
    return wrap_normalize_cell(self, cell, normalized);
}

static void
sylves_wrap_modifier_destroy(SylvesGrid* grid) {
    struct SylvesWrapModifier* self = (struct SylvesWrapModifier*)grid;
    sylves_grid_destroy(self->wrapped);
    sylves_free(self);
}

static void
sylves_wrap_modifier_init_vtable(SylvesGridVTable* vtable) {
    memset(vtable, 0, sizeof(SylvesGridVTable));
    vtable->try_move = sylves_wrap_modifier_try_move;
    vtable->destroy = sylves_wrap_modifier_destroy;
}

void
sylves_wrap_modifier_vtable_init(SylvesGridVTable* vtable) {
    sylves_wrap_modifier_init_vtable(vtable);
}

