/**
 * @file cube_cell_type.h
 * @brief Cube cell type definition
 */

#ifndef SYLVES_CUBE_CELL_TYPE_H
#define SYLVES_CUBE_CELL_TYPE_H

#include "sylves/cell_type.h"
#include "sylves/cube_grid.h"  /* Get cube direction and corner enums from here */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the cube cell type instance
 * @return Pointer to the cube cell type
 */
const SylvesCellType* sylves_cube_cell_type_get(void);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_CUBE_CELL_TYPE_H */
