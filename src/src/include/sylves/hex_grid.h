/**
 * @file hex_grid.h
 * @brief hex grid - to be implemented
 */

#ifndef SYLVES_HEX_GRID_H
#define SYLVES_HEX_GRID_H

#include "types.h"
#include "grid.h"
#include "hex_rotation.h"

/* Hex grid orientation */
typedef enum {
    SYLVES_HEX_ORIENTATION_FLAT_TOP = 0,
    SYLVES_HEX_ORIENTATION_POINTY_TOP = 1,
} SylvesHexOrientation;

/* Hex directions (axial order: E, NE, NW, W, SW, SE) */
typedef enum {
    SYLVES_HEX_DIR_E = 0,
    SYLVES_HEX_DIR_NE = 1,
    SYLVES_HEX_DIR_NW = 2,
    SYLVES_HEX_DIR_W = 3,
    SYLVES_HEX_DIR_SW = 4,
    SYLVES_HEX_DIR_SE = 5,
    SYLVES_HEX_DIR_COUNT = 6,
} SylvesHexDir;

/* Creation */
SylvesGrid* sylves_hex_grid_create(SylvesHexOrientation orient, double cell_size);
SylvesGrid* sylves_hex_grid_create_bounded(SylvesHexOrientation orient, double cell_size,
                                           int min_q, int min_r, int max_q, int max_r);

/* Coordinate conversions (axial q,r  <-> cube x,y,z with x+y+z=0) */
void sylves_hex_axial_to_cube(int q, int r, int* x, int* y, int* z);
void sylves_hex_cube_to_axial(int x, int y, int z, int* q, int* r);

/* Optional common offset conversions (even-q) for 2D usage */
void sylves_hex_axial_to_offset_evenq(int q, int r, int* col, int* row);
void sylves_hex_offset_evenq_to_axial(int col, int row, int* q, int* r);

/* Symmetry helpers */
bool sylves_hex_grid_try_move_by_offset(const SylvesGrid* grid, SylvesCell cell, 
                                         SylvesVector3Int offset, SylvesCell* dest);
SylvesCellDir sylves_hex_grid_parallel_transport(const SylvesGrid* grid, SylvesCell cell, 
                                                 SylvesCell dest, SylvesCellDir dir);
bool sylves_hex_grid_try_apply_symmetry(const SylvesGrid* grid, 
                                        SylvesGridSymmetry symmetry, SylvesCell cell,
                                        SylvesCell* dest, SylvesHexRotation* rotation);

/* Helper functions for hex/triangle grid interaction */
void sylves_hex_get_child_triangles(SylvesCell hex_cell, SylvesCell triangles[6]);
SylvesCell sylves_triangle_get_hex_parent(SylvesCell tri_cell);


#endif /* SYLVES_HEX_GRID_H */
