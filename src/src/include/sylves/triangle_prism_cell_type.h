/**
 * @file triangle_prism_cell_type.h
 * @brief Triangle prism cell type definition
 */

#ifndef SYLVES_TRIANGLE_PRISM_CELL_TYPE_H
#define SYLVES_TRIANGLE_PRISM_CELL_TYPE_H

#include "sylves/cell_type.h"
#include "sylves/triangle_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Triangle prism cell directions
 * 
 * Directions 0-5 map to the 6 hex-like directions (triangles have 3 actual edges but 6 potential dirs)
 * Directions 6-7 are up/down (forward/back in z-axis)
 */
typedef enum {
    SYLVES_TRIANGLE_PRISM_DIR_RIGHT = 0,
    SYLVES_TRIANGLE_PRISM_DIR_UP_RIGHT = 1,
    SYLVES_TRIANGLE_PRISM_DIR_UP_LEFT = 2,
    SYLVES_TRIANGLE_PRISM_DIR_LEFT = 3,
    SYLVES_TRIANGLE_PRISM_DIR_DOWN_LEFT = 4,
    SYLVES_TRIANGLE_PRISM_DIR_DOWN_RIGHT = 5,
    SYLVES_TRIANGLE_PRISM_DIR_FORWARD = 6,  /* +Z direction */
    SYLVES_TRIANGLE_PRISM_DIR_BACK = 7      /* -Z direction */
} SylvesTrianglePrismDir;

/**
 * @brief Get the triangle prism cell type instance
 * @param flat_topped Whether the triangles are flat-topped or flat-sided
 * @return Pointer to the triangle prism cell type
 */
const SylvesCellType* sylves_triangle_prism_cell_type_get(bool flat_topped);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_TRIANGLE_PRISM_CELL_TYPE_H */
