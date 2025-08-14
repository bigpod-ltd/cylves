#include "sylves/grid_modifier.h"
#include "internal/grid_internal.h"
#include <stdlib.h>
#include <stdbool.h>

// Forward declarations
static bool forward_is_2d(const SylvesGrid* grid);
static bool forward_is_3d(const SylvesGrid* grid);
static bool forward_is_planar(const SylvesGrid* grid);
static bool forward_is_repeating(const SylvesGrid* grid);
static bool forward_is_orientable(const SylvesGrid* grid);
static bool forward_is_finite(const SylvesGrid* grid);
static void modifier_destroy(SylvesGrid* grid);

// Forward declare the vtable
static const SylvesGridVTable modifier_vtable;

// Base modifier destruction
static void modifier_destroy(SylvesGrid* grid) {
    if (grid && grid->type == SYLVES_GRID_TYPE_MODIFIER) {
        SylvesGridModifier* modifier = (SylvesGridModifier*)grid;
        if (modifier->modifier_data) {
            free(modifier->modifier_data);
        }
        free(modifier);
    }
}

// Create a base grid modifier that wraps another grid
SylvesGrid* sylves_grid_modifier_create(
    SylvesGrid* underlying) {

    SylvesGridModifier* modifier = (SylvesGridModifier*)malloc(sizeof(SylvesGridModifier));
    if (!modifier) {
        return NULL;
    }
    
    modifier->base.type = SYLVES_GRID_TYPE_MODIFIER;
    modifier->base.vtable = &modifier_vtable;
    modifier->base.bound = underlying->bound;
    modifier->base.data = NULL;
    modifier->underlying = underlying;
    modifier->modifier_data = NULL;

    return (SylvesGrid*)modifier;
}

// Forwarding function implementations
static bool forward_is_2d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_2d(modifier->underlying);
}

static bool forward_is_3d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_3d(modifier->underlying);
}

static bool forward_is_planar(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_planar(modifier->underlying);
}

static bool forward_is_repeating(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_repeating(modifier->underlying);
}

static bool forward_is_orientable(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_orientable(modifier->underlying);
}

static bool forward_is_finite(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_finite(modifier->underlying);
}

// Implement a grid modifier that dispatches calls to the underlying grid
static const SylvesGridVTable modifier_vtable = {
    .destroy = modifier_destroy,
    .is_2d = forward_is_2d,
    .is_3d = forward_is_3d,
    .is_planar = forward_is_planar,
    .is_repeating = forward_is_repeating,
    .is_orientable = forward_is_orientable,
    .is_finite = forward_is_finite,
    // ... Other forwarders will be added as needed
};
