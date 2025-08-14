/**
 * @file cell_type.h
 * @brief Cell type interface for Sylves library
 */

#ifndef SYLVES_CELL_TYPE_H
#define SYLVES_CELL_TYPE_H

#include "types.h"


/**
 * @brief Get the number of directions for a cell type
 */
int sylves_cell_type_get_dir_count(const SylvesCellType* cell_type);

/**
 * @brief Get the number of corners for a cell type
 */
int sylves_cell_type_get_corner_count(const SylvesCellType* cell_type);

/**
 * @brief Get dimension (2D or 3D)
 */
int sylves_cell_type_get_dimension(const SylvesCellType* cell_type);

/**
 * @brief Get all directions for this cell type
 */
int sylves_cell_type_get_dirs(const SylvesCellType* cell_type, 
                              SylvesCellDir* dirs, size_t max_dirs);

/**
 * @brief Get all corners for this cell type
 */
int sylves_cell_type_get_corners(const SylvesCellType* cell_type,
                                 SylvesCellCorner* corners, size_t max_corners);

/**
 * @brief Get all rotations/reflections for this cell type
 */
int sylves_cell_type_get_rotations(const SylvesCellType* cell_type,
                                   SylvesCellRotation* rotations, size_t max_rotations,
                                   bool include_reflections);

/**
 * @brief Get opposite direction if it exists
 */
bool sylves_cell_type_invert_dir(const SylvesCellType* cell_type,
                                 SylvesCellDir dir, SylvesCellDir* inverse_dir);

/**
 * @brief Rotate a direction by a rotation
 */
SylvesCellDir sylves_cell_type_rotate_dir(const SylvesCellType* cell_type,
                                          SylvesCellDir dir, SylvesCellRotation rotation);

/**
 * @brief Rotate a corner by a rotation
 */
SylvesCellCorner sylves_cell_type_rotate_corner(const SylvesCellType* cell_type,
                                                SylvesCellCorner corner, SylvesCellRotation rotation);

/**
 * @brief Compose two rotations
 */
SylvesCellRotation sylves_cell_type_multiply_rotations(const SylvesCellType* cell_type,
                                                       SylvesCellRotation a, SylvesCellRotation b);

/**
 * @brief Get the inverse of a rotation
 */
SylvesCellRotation sylves_cell_type_invert_rotation(const SylvesCellType* cell_type,
                                                    SylvesCellRotation rotation);

/**
 * @brief Get the identity rotation
 */
SylvesCellRotation sylves_cell_type_get_identity_rotation(const SylvesCellType* cell_type);

/**
 * @brief Get the corner position in the canonical cell shape
 */
SylvesVector3 sylves_cell_type_get_corner_position(const SylvesCellType* cell_type,
                                                   SylvesCellCorner corner);

/**
 * @brief Get the transformation matrix for a rotation
 */
SylvesMatrix4x4 sylves_cell_type_get_rotation_matrix(const SylvesCellType* cell_type,
                                                     SylvesCellRotation rotation);

/**
 * @brief Get connection information for moving between cells with a rotation
 */
void sylves_cell_type_get_connection(const SylvesCellType* cell_type,
                                     SylvesCellDir dir, SylvesCellRotation rotation,
                                     SylvesCellDir* result_dir, SylvesConnection* connection);

/**
 * @brief Try to find a rotation that transforms one direction to another with given connection
 */
bool sylves_cell_type_try_get_rotation(const SylvesCellType* cell_type,
                                       SylvesCellDir from_dir, SylvesCellDir to_dir,
                                       const SylvesConnection* connection,
                                       SylvesCellRotation* rotation);

/**
 * @brief Get name of the cell type
 */
const char* sylves_cell_type_get_name(const SylvesCellType* cell_type);

/**
 * @brief Destroy a cell type
 */
void sylves_cell_type_destroy(SylvesCellType* cell_type);

/* Built-in cell type factories */

/**
 * @brief Create a square cell type
 */
SylvesCellType* sylves_square_cell_type_create(void);

/**
 * @brief Create a hex cell type
 * @param is_flat_topped true for flat-topped orientation, false for pointy-topped
 */
SylvesCellType* sylves_hex_cell_type_create(bool is_flat_topped);

/**
 * @brief Create a triangle cell type  
 * @param is_flat_topped true for flat-topped orientation, false for flat-sides
 */
SylvesCellType* sylves_triangle_cell_type_create(bool is_flat_topped);

/**
 * @brief Create a cube cell type
 */
SylvesCellType* sylves_cube_cell_type_create(void);


#endif /* SYLVES_CELL_TYPE_H */
