/**
 * @file ngon_prism_cell_type.h
 * @brief N-gon prism cell type definition
 */

#ifndef SYLVES_NGON_PRISM_CELL_TYPE_H
#define SYLVES_NGON_PRISM_CELL_TYPE_H

#include "sylves/cell_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the n-gon prism cell type instance
 * @param n Number of sides for the base polygon (must be >= 3)
 * @return Pointer to the n-gon prism cell type, or NULL if n < 3
 */
const SylvesCellType* sylves_ngon_prism_cell_type_get(int n);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_NGON_PRISM_CELL_TYPE_H */
