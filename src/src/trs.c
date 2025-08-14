/**
 * @file trs.c
 * @brief Transform-Rotation-Scale operations implementation
 */

#include "sylves/trs.h"
#include "sylves/matrix.h"
#include "sylves/quaternion.h"
#include <math.h>

SylvesTRS sylves_trs_create(SylvesVector3 position, SylvesQuaternion rotation, SylvesVector3 scale) {
    SylvesTRS trs;
    trs.position = position;
    trs.rotation = sylves_matrix4x4_from_quaternion(rotation);
    trs.scale = scale;
    return trs;
}

SylvesTRS sylves_trs_create_from_matrix(SylvesVector3 position, SylvesMatrix4x4 rotation, SylvesVector3 scale) {
    SylvesTRS trs;
    trs.position = position;
    trs.rotation = rotation;
    trs.scale = scale;
    return trs;
}

SylvesTRS sylves_trs_identity(void) {
    SylvesTRS trs;
    trs.position = sylves_vector3_zero();
    trs.rotation = sylves_matrix4x4_identity();
    trs.scale = sylves_vector3_create(1.0, 1.0, 1.0);
    return trs;
}

SylvesTRS sylves_trs_from_position(SylvesVector3 position) {
    SylvesTRS trs = sylves_trs_identity();
    trs.position = position;
    return trs;
}

SylvesTRS sylves_trs_from_rotation(SylvesQuaternion rotation) {
    SylvesTRS trs = sylves_trs_identity();
    trs.rotation = sylves_matrix4x4_from_quaternion(rotation);
    return trs;
}

SylvesTRS sylves_trs_from_scale(SylvesVector3 scale) {
    SylvesTRS trs = sylves_trs_identity();
    trs.scale = scale;
    return trs;
}

SylvesMatrix4x4 sylves_trs_to_matrix(SylvesTRS trs) {
    return sylves_matrix4x4_from_trs(trs);
}

SylvesVector3 sylves_trs_transform_point(SylvesTRS trs, SylvesVector3 point) {
    // Scale first
    SylvesVector3 scaled = sylves_vector3_multiply(point, trs.scale);
    
    // Then rotate
    SylvesVector3 rotated = sylves_matrix4x4_multiply_point(&trs.rotation, scaled);
    
    // Finally translate
    return sylves_vector3_add(rotated, trs.position);
}

SylvesVector3 sylves_trs_transform_vector(SylvesTRS trs, SylvesVector3 vector) {
    // Scale first
    SylvesVector3 scaled = sylves_vector3_multiply(vector, trs.scale);
    
    // Then rotate (vectors don't get translated)
    return sylves_matrix4x4_multiply_vector(&trs.rotation, scaled);
}

SylvesVector3 sylves_trs_transform_direction(SylvesTRS trs, SylvesVector3 direction) {
    // Directions only get rotated, not scaled or translated
    return sylves_matrix4x4_multiply_vector(&trs.rotation, direction);
}

SylvesTRS sylves_trs_inverse(SylvesTRS trs) {
    SylvesTRS result;
    
    // Inverse scale
    result.scale = sylves_vector3_create(
        fabs(trs.scale.x) > 1e-6 ? 1.0 / trs.scale.x : 0.0,
        fabs(trs.scale.y) > 1e-6 ? 1.0 / trs.scale.y : 0.0,
        fabs(trs.scale.z) > 1e-6 ? 1.0 / trs.scale.z : 0.0
    );
    
    // Inverse rotation (transpose for orthogonal matrix)
    result.rotation = sylves_matrix4x4_transpose(&trs.rotation);
    
    // Inverse translation
    SylvesVector3 inv_pos = sylves_vector3_scale(trs.position, -1.0);
    inv_pos = sylves_vector3_multiply(inv_pos, result.scale);
    result.position = sylves_matrix4x4_multiply_point(&result.rotation, inv_pos);
    
    return result;
}

SylvesTRS sylves_trs_combine(SylvesTRS a, SylvesTRS b) {
    SylvesTRS result;
    
    // Combine scales (multiply)
    result.scale = sylves_vector3_multiply(a.scale, b.scale);
    
    // Combine rotations (multiply matrices)
    result.rotation = sylves_matrix4x4_multiply(&a.rotation, &b.rotation);
    
    // Combine positions (transform b's position by a)
    result.position = sylves_trs_transform_point(a, b.position);
    
    return result;
}

SylvesTRS sylves_trs_lerp(SylvesTRS a, SylvesTRS b, double t) {
    SylvesTRS result;
    
    // Lerp position
    result.position = sylves_vector3_lerp(a.position, b.position, t);
    
    // Lerp scale
    result.scale = sylves_vector3_lerp(a.scale, b.scale, t);
    
    // For rotation, we need to extract quaternions and slerp
    // This is a simplified version - for production code, you'd want proper quaternion extraction
    // For now, we'll just use linear interpolation of the matrix elements
    for (int i = 0; i < 16; i++) {
        result.rotation.m[i] = a.rotation.m[i] + (b.rotation.m[i] - a.rotation.m[i]) * t;
    }
    
    // Re-orthogonalize the rotation matrix
    // Extract the basis vectors
    SylvesVector3 right = sylves_vector3_create(result.rotation.m[0], result.rotation.m[1], result.rotation.m[2]);
    SylvesVector3 up = sylves_vector3_create(result.rotation.m[4], result.rotation.m[5], result.rotation.m[6]);
    SylvesVector3 forward = sylves_vector3_create(result.rotation.m[8], result.rotation.m[9], result.rotation.m[10]);
    
    // Normalize and re-orthogonalize
    right = sylves_vector3_normalize(right);
    forward = sylves_vector3_normalize(sylves_vector3_subtract(forward, sylves_vector3_scale(right, sylves_vector3_dot(forward, right))));
    up = sylves_vector3_cross(forward, right);
    
    // Rebuild the matrix
    result.rotation.m[0] = right.x;
    result.rotation.m[1] = right.y;
    result.rotation.m[2] = right.z;
    
    result.rotation.m[4] = up.x;
    result.rotation.m[5] = up.y;
    result.rotation.m[6] = up.z;
    
    result.rotation.m[8] = forward.x;
    result.rotation.m[9] = forward.y;
    result.rotation.m[10] = forward.z;
    
    return result;
}

bool sylves_trs_approx_equal(SylvesTRS a, SylvesTRS b, double epsilon) {
    // Check position
    if (!sylves_vector3_approx_equal(a.position, b.position, epsilon)) {
        return false;
    }
    
    // Check scale
    if (!sylves_vector3_approx_equal(a.scale, b.scale, epsilon)) {
        return false;
    }
    
    // Check rotation matrix elements
    for (int i = 0; i < 16; i++) {
        if (fabs(a.rotation.m[i] - b.rotation.m[i]) > epsilon) {
            return false;
        }
    }
    
    return true;
}
