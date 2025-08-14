#include "sylves/transform_modifier.h"
#include "sylves/matrix.h"
#include "sylves/vector.h"
#include "sylves/aabb.h"
#include "internal/grid_internal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Internal data for transform modifier
typedef struct {
    SylvesMatrix4x4 transform;
    SylvesMatrix4x4 inverse_transform;
} TransformModifierData;

// Forward declarations for transform-specific operations
static void transform_modifier_destroy(SylvesGrid* grid);
static SylvesVector3 transform_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static SylvesVector3 transform_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner);
static bool transform_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);
static SylvesError transform_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb);

// Forward declare vtable
static const SylvesGridVTable transform_modifier_vtable;

// Transform modifier destruction
static void transform_modifier_destroy(SylvesGrid* grid) {
    if (grid && grid->type == SYLVES_GRID_TYPE_MODIFIER) {
        SylvesGridModifier* modifier = (SylvesGridModifier*)grid;
        if (modifier->modifier_data) {
            free(modifier->modifier_data);
        }
        free(modifier);
    }
}

// Create a transform modifier grid
SylvesGrid* sylves_transform_modifier_create(SylvesGrid* underlying, const SylvesMatrix4x4* transform) {
    if (!underlying || !transform) {
        return NULL;
    }

    SylvesGridModifier* modifier = (SylvesGridModifier*)malloc(sizeof(SylvesGridModifier));
    if (!modifier) {
        return NULL;
    }

    TransformModifierData* data = (TransformModifierData*)malloc(sizeof(TransformModifierData));
    if (!data) {
        free(modifier);
        return NULL;
    }

    // Store the transformation and its inverse
    data->transform = *transform;
    if (!sylves_matrix4x4_invert(transform, &data->inverse_transform)) {
        free(data);
        free(modifier);
        return NULL;
    }

    // Initialize the modifier
    modifier->base.type = SYLVES_GRID_TYPE_MODIFIER;
    modifier->base.vtable = &transform_modifier_vtable;
    modifier->base.bound = underlying->bound;
    modifier->base.data = NULL;
    modifier->underlying = underlying;
    modifier->modifier_data = data;

    return (SylvesGrid*)modifier;
}

// Get the transformation matrix from a transform modifier
const SylvesMatrix4x4* sylves_transform_modifier_get_transform(const SylvesGrid* grid) {
    if (grid && grid->type == SYLVES_GRID_TYPE_MODIFIER) {
        const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
        const TransformModifierData* data = (const TransformModifierData*)modifier->modifier_data;
        return &data->transform;
    }
    return NULL;
}

// Transform-specific implementations
static SylvesVector3 transform_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const TransformModifierData* data = (const TransformModifierData*)modifier->modifier_data;
    
    // Get center from underlying grid
    SylvesVector3 center = sylves_grid_get_cell_center(modifier->underlying, cell);
    
    // Transform the point
    return sylves_matrix4x4_multiply_point(&data->transform, center);
}

static SylvesVector3 transform_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const TransformModifierData* data = (const TransformModifierData*)modifier->modifier_data;
    
    // Get corner from underlying grid
    SylvesVector3 corner_pos = sylves_grid_get_cell_corner(modifier->underlying, cell, corner);
    
    // Transform the point
    return sylves_matrix4x4_multiply_point(&data->transform, corner_pos);
}

static bool transform_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const TransformModifierData* data = (const TransformModifierData*)modifier->modifier_data;
    
    // Transform position to underlying grid's coordinate system
    SylvesVector3 inv_position = sylves_matrix4x4_multiply_point(&data->inverse_transform, position);
    
    // Find cell in underlying grid
    return sylves_grid_find_cell(modifier->underlying, inv_position, cell);
}

static SylvesError transform_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    const TransformModifierData* data = (const TransformModifierData*)modifier->modifier_data;
    
    // Get AABB from underlying grid
    SylvesError err = sylves_grid_get_cell_aabb(modifier->underlying, cell, aabb);
    if (err != SYLVES_SUCCESS) {
        return err;
    }
    
    // Transform the AABB
    *aabb = sylves_aabb_transform(*aabb, &data->transform);
    return SYLVES_SUCCESS;
}

// Forward property queries to underlying grid
static bool transform_is_2d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_2d(modifier->underlying);
}

static bool transform_is_3d(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_3d(modifier->underlying);
}

static bool transform_is_planar(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_planar(modifier->underlying);
}

static bool transform_is_repeating(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_repeating(modifier->underlying);
}

static bool transform_is_orientable(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_orientable(modifier->underlying);
}

static bool transform_is_finite(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_finite(modifier->underlying);
}

static int transform_get_coordinate_dimension(const SylvesGrid* grid) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_coordinate_dimension(modifier->underlying);
}

// Forward cell operations to underlying grid
static bool transform_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_is_cell_in_grid(modifier->underlying, cell);
}

static const SylvesCellType* transform_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_type(modifier->underlying, cell);
}

// Forward topology operations to underlying grid
static bool transform_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                              SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_try_move(modifier->underlying, cell, dir, dest, inverse_dir, connection);
}

static int transform_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                                  SylvesCellDir* dirs, size_t max_dirs) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_dirs(modifier->underlying, cell, dirs, max_dirs);
}

static int transform_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                     SylvesCellCorner* corners, size_t max_corners) {
    const SylvesGridModifier* modifier = (const SylvesGridModifier*)grid;
    return sylves_grid_get_cell_corners(modifier->underlying, cell, corners, max_corners);
}

// Transform modifier vtable
static const SylvesGridVTable transform_modifier_vtable = {
    .destroy = transform_modifier_destroy,
    
    // Properties - forward to underlying
    .is_2d = transform_is_2d,
    .is_3d = transform_is_3d,
    .is_planar = transform_is_planar,
    .is_repeating = transform_is_repeating,
    .is_orientable = transform_is_orientable,
    .is_finite = transform_is_finite,
    .get_coordinate_dimension = transform_get_coordinate_dimension,
    
    // Cell operations - forward to underlying
    .is_cell_in_grid = transform_is_cell_in_grid,
    .get_cell_type = transform_get_cell_type,
    
    // Topology - forward to underlying (unchanged by transform)
    .try_move = transform_try_move,
    .get_cell_dirs = transform_get_cell_dirs,
    .get_cell_corners = transform_get_cell_corners,
    
    // Position/shape - transform these
    .get_cell_center = transform_get_cell_center,
    .get_cell_corner_pos = transform_get_cell_corner_pos,
    .get_polygon = NULL,  // TODO: Implement polygon transformation
    .get_cell_aabb = transform_get_cell_aabb,
    
    // Queries - transform these
    .find_cell = transform_find_cell,
    .raycast = NULL,  // TODO: Implement raycast transformation
    
    // Index operations - forward to underlying
    .get_index_count = NULL,  // Will use default
    .get_index = NULL,  // Will use default
    .get_cell_by_index = NULL,  // Will use default
};
