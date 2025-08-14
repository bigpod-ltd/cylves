#ifndef SYLVES_TRANSFORM_MODIFIER_H
#define SYLVES_TRANSFORM_MODIFIER_H

#include "sylves/grid.h"
#include "sylves/grid_modifier.h"
#include "sylves/matrix.h"

/**
 * @brief Create a transform modifier that applies a 3D transformation to a grid
 * 
 * The transform modifier changes the world space positioning of the grid by a linear transform,
 * leaving all cell relationships and topology unchanged.
 * 
 * @param underlying The grid to transform
 * @param transform The transformation matrix to apply
 * @return New transformed grid, or NULL on error
 */
SylvesGrid* sylves_transform_modifier_create(SylvesGrid* underlying, const SylvesMatrix4x4* transform);

/**
 * @brief Get the transformation matrix from a transform modifier
 * 
 * @param grid The transform modifier grid
 * @return Pointer to the transformation matrix, or NULL if not a transform modifier
 */
const SylvesMatrix4x4* sylves_transform_modifier_get_transform(const SylvesGrid* grid);

#endif // SYLVES_TRANSFORM_MODIFIER_H
