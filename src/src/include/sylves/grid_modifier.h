#ifndef SYLVES_GRID_MODIFIER_H
#define SYLVES_GRID_MODIFIER_H

#include "sylves/types.h"
#include "sylves/grid.h"
#include "sylves/errors.h"
#include "../../internal/grid_internal.h"
// Forward declaration
typedef struct SylvesGridModifier SylvesGridModifier;

// Base modifier structure - inherits from SylvesGrid
struct SylvesGridModifier {
    SylvesGrid base;           // Inherits from SylvesGrid
    SylvesGrid* underlying;    // The wrapped grid
    void* modifier_data;       // Modifier-specific data
};

// Helper to get the underlying grid from any modifier
static inline SylvesGrid* sylves_grid_modifier_get_underlying(const SylvesGrid* grid) {
    if (grid && grid->type == SYLVES_GRID_TYPE_MODIFIER) {
        return ((const SylvesGridModifier*)grid)->underlying;
    }
    return NULL;
}

#endif // SYLVES_GRID_MODIFIER_H
