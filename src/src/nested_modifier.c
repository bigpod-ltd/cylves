/**
 * @file nested_modifier.c
 * @brief Implementation of the Nested modifier for hierarchical grids
 * 
 * This file provides the implementation for the Nested Modifier, a grid
 * modifier that creates hierarchical grids by replacing each cell of a base
 * grid with an entire child grid.
 */

#include "sylves/nested_modifier.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "grid_internal.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct SylvesNestedModifier {
    SylvesGrid base;
    SylvesGrid* outer;
    SylvesGrid* inner;
};

SylvesGrid*
sylves_nested_modifier_create(
    SylvesGrid* base_grid,
    SylvesGrid* child_grid
) {
    if (!base_grid || !child_grid) return NULL;

    struct SylvesNestedModifier* grid = (struct SylvesNestedModifier*)sylves_alloc(sizeof(struct SylvesNestedModifier));
    grid->outer = base_grid;
    grid->inner = child_grid;

    return (SylvesGrid*)grid;
}

static void
sylves_nested_modifier_destroy(SylvesGrid* grid) {
    struct SylvesNestedModifier* self = (struct SylvesNestedModifier*)grid;
    sylves_grid_destroy(self->outer);
    sylves_grid_destroy(self->inner);
    sylves_free(self);
}

SylvesCell
sylves_nested_cell_create(
    SylvesCell base_cell,
    SylvesCell child_cell
) {
    // Encode nested cell using bit packing in the x, y, z components
    // Use upper 16 bits for base cell, lower 16 bits for child cell
    SylvesCell nested_cell;
    nested_cell.x = (base_cell.x << 16) | (child_cell.x & 0xFFFF);
    nested_cell.y = (base_cell.y << 16) | (child_cell.y & 0xFFFF);
    nested_cell.z = (base_cell.z << 16) | (child_cell.z & 0xFFFF);
    return nested_cell;
}

SylvesCell
sylves_nested_cell_get_base(SylvesCell nested_cell) {
    // Extract base cell from upper 16 bits
    SylvesCell base_cell;
    base_cell.x = nested_cell.x >> 16;
    base_cell.y = nested_cell.y >> 16;
    base_cell.z = nested_cell.z >> 16;
    return base_cell;
}

SylvesCell
sylves_nested_cell_get_child(SylvesCell nested_cell) {
    // Extract child cell from lower 16 bits (sign-extend if negative)
    SylvesCell child_cell;
    child_cell.x = (int16_t)(nested_cell.x & 0xFFFF);
    child_cell.y = (int16_t)(nested_cell.y & 0xFFFF);
    child_cell.z = (int16_t)(nested_cell.z & 0xFFFF);
    return child_cell;
}

const SylvesGrid*
sylves_nested_modifier_get_base_grid(const SylvesGrid* grid) {
    const struct SylvesNestedModifier* self = (const struct SylvesNestedModifier*)grid;
    return self->outer;
}

const SylvesGrid*
sylves_nested_modifier_get_child_grid(const SylvesGrid* grid) {
    const struct SylvesNestedModifier* self = (const struct SylvesNestedModifier*)grid;
    return self->inner;
}

static void
sylves_nested_modifier_init_vtable(SylvesGridVTable* vtable) {
    memset(vtable, 0, sizeof(SylvesGridVTable));
    vtable->destroy = sylves_nested_modifier_destroy;
}

void
sylves_nested_modifier_vtable_init(SylvesGridVTable* vtable) {
    sylves_nested_modifier_init_vtable(vtable);
}

