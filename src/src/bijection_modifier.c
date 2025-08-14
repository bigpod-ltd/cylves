/**
 * @file bijection_modifier.c
 * @brief Implementation of the Bijection modifier for remapping grid coordinates
 *
 * This file provides the implementation for the Bijection Modifier, allowing
 * coordinate transformations through bijective functions.
 */

#include "sylves/bijection_modifier.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "internal/grid_internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct SylvesBijectionModifier {
    SylvesGrid base;
    SylvesGrid* mapped;
    SylvesCellMapFunc forward;
    SylvesCellMapFunc backward;
};

typedef struct SylvesBijectionModifier SylvesBijectionModifier;

static bool
sylves_bijection_modifier_try_move(
    const SylvesGrid* grid,
    SylvesCell from,
    SylvesCellDir direction,
    SylvesCell* to,
    SylvesCellDir* inverse_dir,
    SylvesConnection* connection
) {
    SylvesBijectionModifier* self = (SylvesBijectionModifier*)grid;
    SylvesCell mapped_from;

    self->backward(from, &mapped_from);

    SylvesCellDir inv_tmp;
    if (!inverse_dir) inverse_dir = &inv_tmp;

    if (!sylves_grid_try_move(self->mapped, mapped_from, direction, &mapped_from, inverse_dir, connection)) {
        return false;
    }

    self->forward(mapped_from, to);
    return true;
}

SylvesGrid*
sylves_bijection_modifier_create(
    SylvesGrid* base_grid,
    SylvesCellMapFunc forward,
    SylvesCellMapFunc backward
) {
    if (!base_grid || !forward || !backward) return NULL;

    SylvesBijectionModifier* grid = (SylvesBijectionModifier*)sylves_alloc(sizeof(SylvesBijectionModifier));
    if (!grid) return NULL;
    grid->mapped = base_grid;
    grid->forward = forward;
    grid->backward = backward;
    grid->base.vtable = NULL;
    grid->base.type = SYLVES_GRID_TYPE_MODIFIER;
    grid->base.bound = NULL;
    grid->base.data = NULL;

    return (SylvesGrid*)grid;
}

static void
sylves_bijection_modifier_destroy(SylvesGrid* grid) {
    SylvesBijectionModifier* self = (SylvesBijectionModifier*)grid;
    sylves_grid_destroy(self->mapped);
    sylves_free(self);
}

static void
sylves_bijection_modifier_init_vtable(SylvesGridVTable* vtable) {
    memset(vtable, 0, sizeof(SylvesGridVTable));
    vtable->try_move = sylves_bijection_modifier_try_move;
    vtable->destroy = sylves_bijection_modifier_destroy;
}

bool
sylves_bijection_modifier_map_forward(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* mapped
) {
    if (!grid || !mapped) return false;

    SylvesBijectionModifier* self = (SylvesBijectionModifier*)grid;
    self->forward(cell, mapped);
    return true;
}

bool
sylves_bijection_modifier_map_backward(
    const SylvesGrid* grid,
    SylvesCell cell,
    SylvesCell* original
) {
    if (!grid || !original) return false;

    SylvesBijectionModifier* self = (SylvesBijectionModifier*)grid;
    self->backward(cell, original);
    return true;
}

void
sylves_bijection_modifier_vtable_init(SylvesGridVTable* vtable) {
    sylves_bijection_modifier_init_vtable(vtable);
}

