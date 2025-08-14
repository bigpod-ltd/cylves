/**
 * @file hex_rotation.h
 * @brief Hex rotation type for symmetry operations
 */

#ifndef SYLVES_HEX_ROTATION_H
#define SYLVES_HEX_ROTATION_H

#include "types.h"


/**
 * @brief Represents a rotation/reflection in hex grid
 * 
 * Encodes rotations (0-5) and reflections (negative values)
 */
typedef struct {
    int value;  /**< Rotation value: 0-5 for rotations, negative for reflections */
} SylvesHexRotation;

/* Creation */
SylvesHexRotation sylves_hex_rotation_identity(void);
SylvesHexRotation sylves_hex_rotation_from_int(int value);

/* Operations */
SylvesVector3Int sylves_hex_rotation_multiply(SylvesHexRotation rotation, SylvesVector3Int v);
SylvesCellDir sylves_hex_rotation_rotate_dir(SylvesHexRotation rotation, SylvesCellDir dir);
SylvesCellCorner sylves_hex_rotation_rotate_corner(SylvesHexRotation rotation, SylvesCellCorner corner);
SylvesHexRotation sylves_hex_rotation_inverse(SylvesHexRotation rotation);
SylvesHexRotation sylves_hex_rotation_compose(SylvesHexRotation a, SylvesHexRotation b);

/* Conversion */
bool sylves_hex_rotation_is_reflection(SylvesHexRotation rotation);
int sylves_hex_rotation_get_rotation_count(SylvesHexRotation rotation);


#endif /* SYLVES_HEX_ROTATION_H */
