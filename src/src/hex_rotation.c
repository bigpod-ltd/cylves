/**
 * @file hex_rotation.c
 * @brief HexRotation implementation for symmetry operations
 */

#include "sylves/hex_rotation.h"
#include <stdlib.h>

SylvesHexRotation sylves_hex_rotation_identity(void) {
    return (SylvesHexRotation){0};
}

SylvesHexRotation sylves_hex_rotation_from_int(int value) {
    return (SylvesHexRotation){value};
}

bool sylves_hex_rotation_is_reflection(SylvesHexRotation rotation) {
    return rotation.value < 0;
}

int sylves_hex_rotation_get_rotation_count(SylvesHexRotation rotation) {
    if (rotation.value >= 0) {
        return rotation.value;
    } else {
        return ~rotation.value;
    }
}

SylvesVector3Int sylves_hex_rotation_multiply(SylvesHexRotation rotation, SylvesVector3Int v) {
    int ir = rotation.value;
    SylvesVector3Int result = v;
    
    if (ir < 0) {
        ir = ~ir;
        /* Apply reflection first - swap y and z */
        int temp = result.y;
        result.y = result.z;
        result.z = temp;
    }
    
    /* Apply rotation */
    switch (ir) {
        case 0: /* Identity */
            break;
        case 1: /* 60 degrees CCW */
            result = (SylvesVector3Int){-v.y, -v.z, -v.x};
            break;
        case 2: /* 120 degrees CCW */
            result = (SylvesVector3Int){v.z, v.x, v.y};
            break;
        case 3: /* 180 degrees */
            result = (SylvesVector3Int){-v.x, -v.y, -v.z};
            break;
        case 4: /* 240 degrees CCW */
            result = (SylvesVector3Int){v.y, v.z, v.x};
            break;
        case 5: /* 300 degrees CCW */
            result = (SylvesVector3Int){-v.z, -v.x, -v.y};
            break;
    }
    
    return result;
}

SylvesCellDir sylves_hex_rotation_rotate_dir(SylvesHexRotation rotation, SylvesCellDir dir) {
    int ir = rotation.value;
    
    if (ir < 0) {
        ir = ~ir;
        /* Apply reflection - invert direction order */
        dir = (5 - dir);
    }
    
    /* Apply rotation */
    return (dir + ir) % 6;
}

SylvesCellCorner sylves_hex_rotation_rotate_corner(SylvesHexRotation rotation, SylvesCellCorner corner) {
    /* For hex grids, corners are rotated the same way as directions */
    return sylves_hex_rotation_rotate_dir(rotation, corner);
}

SylvesHexRotation sylves_hex_rotation_inverse(SylvesHexRotation rotation) {
    int ir = rotation.value;
    
    if (ir >= 0) {
        /* Inverse of rotation r is rotation -r mod 6 */
        return (SylvesHexRotation){(6 - ir) % 6};
    } else {
        /* Reflections are self-inverse */
        return rotation;
    }
}

SylvesHexRotation sylves_hex_rotation_compose(SylvesHexRotation a, SylvesHexRotation b) {
    bool a_refl = sylves_hex_rotation_is_reflection(a);
    bool b_refl = sylves_hex_rotation_is_reflection(b);
    int a_rot = sylves_hex_rotation_get_rotation_count(a);
    int b_rot = sylves_hex_rotation_get_rotation_count(b);
    
    if (!a_refl && !b_refl) {
        /* Two rotations compose to a rotation */
        return (SylvesHexRotation){(a_rot + b_rot) % 6};
    } else if (a_refl && b_refl) {
        /* Two reflections compose to a rotation */
        return (SylvesHexRotation){(a_rot - b_rot + 6) % 6};
    } else if (a_refl) {
        /* Reflection followed by rotation */
        return (SylvesHexRotation){~((a_rot - b_rot + 6) % 6)};
    } else {
        /* Rotation followed by reflection */
        return (SylvesHexRotation){~((a_rot + b_rot) % 6)};
    }
}
