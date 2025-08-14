/**
 * @file hex_prism_cell_type.h
 * @brief Hex prism cell type definition
 */

#ifndef SYLVES_HEX_PRISM_CELL_TYPE_H
#define SYLVES_HEX_PRISM_CELL_TYPE_H

#include "sylves/cell_type.h"
#include "sylves/hex_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hex prism cell directions
 * 
 * Directions 0-5 are the hex base directions
 * Directions 6-7 are up/down (forward/back in z-axis)
 */
typedef enum {
    SYLVES_HEX_PRISM_DIR_RIGHT = 0,
    SYLVES_HEX_PRISM_DIR_UP_RIGHT = 1,
    SYLVES_HEX_PRISM_DIR_UP_LEFT = 2,
    SYLVES_HEX_PRISM_DIR_LEFT = 3,
    SYLVES_HEX_PRISM_DIR_DOWN_LEFT = 4,
    SYLVES_HEX_PRISM_DIR_DOWN_RIGHT = 5,
    SYLVES_HEX_PRISM_DIR_FORWARD = 6,  /* +Z direction */
    SYLVES_HEX_PRISM_DIR_BACK = 7      /* -Z direction */
} SylvesHexPrismDir;

/**
 * @brief Get the hex prism cell type instance
 * @param flat_topped Whether the hex bases are flat-topped or pointy-topped
 * @return Pointer to the hex prism cell type
 */
const SylvesCellType* sylves_hex_prism_cell_type_get(bool flat_topped);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_HEX_PRISM_CELL_TYPE_H */
