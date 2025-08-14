#include "sylves/mask_modifier.h"
#include "internal/grid_internal.h"
#include <stdlib.h>
#include <string.h>

// Internal data for mask modifier
typedef struct {
    SylvesMaskContainsFunc contains_func;
    void* user_data;
    SylvesCell* cells;  // Optional explicit cell list
    size_t cell_count;
    bool owns_cells;    // Whether we need to free the cells array
} MaskModifierData;

// Forward declarations
static void mask_modifier_destroy(SylvesGrid* grid);
static bool mask_is_finite(const SylvesGrid* grid);
static bool mask_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static bool mask_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                         SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static bool mask_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

// Forward declare vtable
static const SylvesGridVTable mask_modifier_vtable;

// Mask modifier destruction
static void mask_modifier_destroy(SylvesGrid* grid) {
    if (grid && grid->type == SYLVES_GRID_TYPE_MODIFIER) {
        SylvesGridModifier* modifier = (SylvesGridModifier*)grid;
        MaskModifierData* data = (MaskModifierData*)modifier->modifier_data;
        if (data) {
            if (data->owns_cells && data->cells) {
                free(data->cells);
            }
            free(data);
        }
        free(modifier);
    }
}

// Create a mask modifier
SylvesGrid* sylves_mask_modifier_create(
    SylvesGrid* underlying,
    SylvesMaskContainsFunc contains_func,
    void* user_data,
    const SylvesCell* cells,
    size_t cell_count) {
    
    if (!underlying || !contains_func) {
        return NULL;
    }

    SylvesGridModifier* modifier = (SylvesGridModifier*)malloc(sizeof(SylvesGridModifier));
    if (!modifier) {
        return NULL;
    }

    MaskModifierData* data = (MaskModifierData*)malloc(sizeof(MaskModifierData));
    if (!data) {
        free(modifier);
        return NULL;
    }

    // Initialize mask data
    data->contains_func = contains_func;
    data->user_data = user_data;
    data->cells = NULL;
    data->cell_count = 0;
    data->owns_cells = false;

    // Copy cells if provided
    if (cells && cell_count > 0) {
        data->cells = (SylvesCell*)malloc(sizeof(SylvesCell) * cell_count);
        if (!data->cells) {
            free(data);
            free(modifier);
            return NULL;
        }
        memcpy(data->cells, cells, sizeof(SylvesCell) * cell_count);
        data->cell_count = cell_count;
        data->owns_cells = true;
    }

    // Initialize the modifier
    modifier->base.type = SYLVES_GRID_TYPE_MODIFIER;
    modifier->base.vtable = &mask_modifier_vtable;
    modifier->base.bound = underlying->bound;
    modifier->base.data = NULL;
    modifier->underlying = underlying;
    modifier->modifier_data = data;

    return (SylvesGrid*)modifier;
}

// Helper function to check if a cell is in a cell array
static bool cell_array_contains(SylvesCell cell, void* user_data) {
    MaskModifierData* data = (MaskModifierData*)user_data;
    for (size_t i = 0; i < data->cell_count; i++) {
        if (data->cells[i].x == cell.x &&
            data->cells[i].y == cell.y &&
            data->cells[i].z == cell.z) {
            return true;
        }
    }
    return false;
}

// Create mask modifier from explicit cell set
SylvesGrid* sylves_mask_modifier_create_from_cells(
    SylvesGrid* underlying,
    const SylvesCell* cells,
    size_t cell_count) {
    
    if (!underlying || !cells || cell_count == 0) {
        return NULL;
    }

    // Create mask data that will be used for the contains function
    MaskModifierData* temp_data = (MaskModifierData*)malloc(sizeof(MaskModifierData));
    if (!temp_data) {
        return NULL;
    }

    temp_data->cells = (SylvesCell*)malloc(sizeof(SylvesCell) * cell_count);
    if (!temp_data->cells) {
        free(temp_data);
        return NULL;
    }
    memcpy(temp_data->cells, cells, sizeof(SylvesCell) * cell_count);
    temp_data->cell_count = cell_count;

    // Create the mask using the cell array contains function
    SylvesGrid* result = sylves_mask_modifier_create(
        underlying,
        cell_array_contains,
        temp_data,
        cells,
        cell_count
    );

    // The mask will have made its own copy, so free the temp data
    free(temp_data->cells);
    free(temp_data);

    return result;
}

// Mask-specific implementations
static bool mask_is_finite(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const MaskModifierData* data = (const MaskModifierData*)modifier->modifier_data;
    
    // If we have an explicit cell list, we're finite
    if (data->cells && data->cell_count > 0) {
        return true;
    }
    
    // Otherwise, we're finite if the underlying grid is finite
    return sylves_grid_is_finite(modifier->underlying);
}

static bool mask_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const MaskModifierData* data = (const MaskModifierData*)modifier->modifier_data;
    
    // First check if cell is in underlying grid
    if (!sylves_grid_is_cell_in_grid(modifier->underlying, cell)) {
        return false;
    }
    
    // Then check if it passes the mask test
    return data->contains_func(cell, data->user_data);
}

static bool mask_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                         SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const MaskModifierData* data = (const MaskModifierData*)modifier->modifier_data;
    
    // Try to move in underlying grid
    if (!sylves_grid_try_move(modifier->underlying, cell, dir, dest, inverse_dir, connection)) {
        return false;
    }
    
    // Check if destination is in mask
    return data->contains_func(*dest, data->user_data);
}

static bool mask_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const MaskModifierData* data = (const MaskModifierData*)modifier->modifier_data;
    
    // Find cell in underlying grid
    if (!sylves_grid_find_cell(modifier->underlying, position, cell)) {
        return false;
    }
    
    // Check if it's in the mask
    return data->contains_func(*cell, data->user_data);
}

// Forward property queries to underlying grid
static bool mask_is_2d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_2d(modifier->underlying);
}

static bool mask_is_3d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_3d(modifier->underlying);
}

static bool mask_is_planar(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_planar(modifier->underlying);
}

static bool mask_is_repeating(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_repeating(modifier->underlying);
}

static bool mask_is_orientable(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_orientable(modifier->underlying);
}

static int mask_get_coordinate_dimension(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_coordinate_dimension(modifier->underlying);
}

// Forward cell type queries to underlying grid
static const SylvesCellType* mask_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_type(modifier->underlying, cell);
}

// Forward position/shape queries to underlying grid
static SylvesVector3 mask_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_center(modifier->underlying, cell);
}

static SylvesVector3 mask_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_corner(modifier->underlying, cell, corner);
}

// Mask modifier vtable
static const SylvesGridVTable mask_modifier_vtable = {
    .destroy = mask_modifier_destroy,
    
    // Properties - mostly forward to underlying
    .is_2d = mask_is_2d,
    .is_3d = mask_is_3d,
    .is_planar = mask_is_planar,
    .is_repeating = mask_is_repeating,
    .is_orientable = mask_is_orientable,
    .is_finite = mask_is_finite,  // May be different from underlying
    .get_coordinate_dimension = mask_get_coordinate_dimension,
    
    // Cell operations - filter by mask
    .is_cell_in_grid = mask_is_cell_in_grid,
    .get_cell_type = mask_get_cell_type,
    
    // Topology - filter by mask
    .try_move = mask_try_move,
    .get_cell_dirs = NULL,  // TODO: Need filtered implementation
    .get_cell_corners = NULL,  // TODO: Need filtered implementation
    
    // Position/shape - forward to underlying
    .get_cell_center = mask_get_cell_center,
    .get_cell_corner_pos = mask_get_cell_corner_pos,
    .get_polygon = NULL,  // Will use default
    .get_cell_aabb = NULL,  // Will use default
    
    // Queries - filter by mask
    .find_cell = mask_find_cell,
    .raycast = NULL,  // TODO: Need filtered implementation
    
    // Index operations
    .get_index_count = NULL,  // TODO: Need filtered implementation
    .get_index = NULL,  // TODO: Need filtered implementation
    .get_cell_by_index = NULL,  // TODO: Need filtered implementation
};
