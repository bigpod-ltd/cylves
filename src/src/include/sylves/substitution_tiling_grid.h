/**
 * @file substitution_tiling_grid.h
 * @brief Substitution tiling grid implementation
 */

#ifndef SYLVES_SUBSTITUTION_TILING_GRID_H
#define SYLVES_SUBSTITUTION_TILING_GRID_H

#include "sylves/grid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Types of substitution tilings supported
 */
typedef enum {
    SYLVES_SUBSTITUTION_PENROSE_RHOMB,     /**< Penrose rhomb tiling (P3) */
    SYLVES_SUBSTITUTION_AMMANN_BEENKER,    /**< Ammann-Beenker (octagonal) tiling */
    SYLVES_SUBSTITUTION_PINWHEEL,          /**< Pinwheel tiling */
    SYLVES_SUBSTITUTION_CHAIR               /**< Chair tiling */
} SylvesSubstitutionType;

/**
 * @brief Create a Penrose rhomb tiling grid
 * 
 * Creates a Penrose P3 tiling using rhombic tiles with
 * inflation/deflation substitution rules.
 * 
 * @param subdivision_depth Number of subdivision iterations (0-10 recommended)
 * @param scale Overall scale of the tiling
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_penrose_rhomb_grid_create(int subdivision_depth, double scale);

/**
 * @brief Create an Ammann-Beenker tiling grid
 * 
 * Creates an octagonal tiling with squares and 45-degree rhombs.
 * 
 * @param subdivision_depth Number of subdivision iterations (0-10 recommended)
 * @param scale Overall scale of the tiling
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_ammann_beenker_grid_create(int subdivision_depth, double scale);

/**
 * @brief Create a generic substitution tiling grid
 * 
 * @param type Type of substitution tiling
 * @param subdivision_depth Number of subdivision iterations
 * @param scale Overall scale of the tiling
 * @return Pointer to the created grid, or NULL on failure
 */
SylvesGrid* sylves_substitution_tiling_grid_create(
    SylvesSubstitutionType type,
    int subdivision_depth,
    double scale
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_SUBSTITUTION_TILING_GRID_H */
