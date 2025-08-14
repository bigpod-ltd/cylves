/**
 * @file matrix.c
 * @brief Matrix operations implementation
 */

#include "sylves/matrix.h"
#include "sylves/quaternion.h"
#include <string.h>
#include <math.h>

/* Helper macros for column-major indexing */
#define M(mat, row, col) ((mat)->m[(col) * 4 + (row)])
#define M_SET(mat, row, col, val) ((mat)->m[(col) * 4 + (row)] = (val))

SylvesMatrix4x4 sylves_matrix4x4_identity(void) {
    SylvesMatrix4x4 m;
    memset(m.m, 0, sizeof(m.m));
    m.m[0] = 1.0;   // m[0][0]
    m.m[5] = 1.0;   // m[1][1]
    m.m[10] = 1.0;  // m[2][2]
    m.m[15] = 1.0;  // m[3][3]
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_zero(void) {
    SylvesMatrix4x4 m;
    memset(m.m, 0, sizeof(m.m));
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_translation(SylvesVector3 translation) {
    SylvesMatrix4x4 m = sylves_matrix4x4_identity();
    m.m[12] = translation.x;  // m[0][3]
    m.m[13] = translation.y;  // m[1][3]
    m.m[14] = translation.z;  // m[2][3]
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_scale(SylvesVector3 scale) {
    SylvesMatrix4x4 m = sylves_matrix4x4_zero();
    m.m[0] = scale.x;   // m[0][0]
    m.m[5] = scale.y;   // m[1][1]
    m.m[10] = scale.z;  // m[2][2]
    m.m[15] = 1.0;      // m[3][3]
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_rotation_x(double angle_radians) {
    SylvesMatrix4x4 m = sylves_matrix4x4_identity();
    double c = cos(angle_radians);
    double s = sin(angle_radians);
    
    m.m[5] = c;    // m[1][1]
    m.m[6] = s;    // m[2][1]
    m.m[9] = -s;   // m[1][2]
    m.m[10] = c;   // m[2][2]
    
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_rotation_y(double angle_radians) {
    SylvesMatrix4x4 m = sylves_matrix4x4_identity();
    double c = cos(angle_radians);
    double s = sin(angle_radians);
    
    m.m[0] = c;    // m[0][0]
    m.m[2] = -s;   // m[2][0]
    m.m[8] = s;    // m[0][2]
    m.m[10] = c;   // m[2][2]
    
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_rotation_z(double angle_radians) {
    SylvesMatrix4x4 m = sylves_matrix4x4_identity();
    double c = cos(angle_radians);
    double s = sin(angle_radians);
    
    m.m[0] = c;    // m[0][0]
    m.m[1] = s;    // m[1][0]
    m.m[4] = -s;   // m[0][1]
    m.m[5] = c;    // m[1][1]
    
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_from_quaternion(SylvesQuaternion q) {
    SylvesMatrix4x4 m = sylves_matrix4x4_identity();
    
    double xx = q.x * q.x;
    double yy = q.y * q.y;
    double zz = q.z * q.z;
    double xy = q.x * q.y;
    double xz = q.x * q.z;
    double yz = q.y * q.z;
    double wx = q.w * q.x;
    double wy = q.w * q.y;
    double wz = q.w * q.z;
    
    m.m[0] = 1.0 - 2.0 * (yy + zz);
    m.m[1] = 2.0 * (xy + wz);
    m.m[2] = 2.0 * (xz - wy);
    
    m.m[4] = 2.0 * (xy - wz);
    m.m[5] = 1.0 - 2.0 * (xx + zz);
    m.m[6] = 2.0 * (yz + wx);
    
    m.m[8] = 2.0 * (xz + wy);
    m.m[9] = 2.0 * (yz - wx);
    m.m[10] = 1.0 - 2.0 * (xx + yy);
    
    return m;
}

SylvesMatrix4x4 sylves_matrix4x4_from_trs(SylvesTRS trs) {
    // Create translation matrix
    SylvesMatrix4x4 t = sylves_matrix4x4_translation(trs.position);
    
    // Apply rotation
    SylvesMatrix4x4 r = trs.rotation;
    
    // Apply scale
    SylvesMatrix4x4 s = sylves_matrix4x4_scale(trs.scale);
    
    // Combine: T * R * S
    SylvesMatrix4x4 temp = sylves_matrix4x4_multiply(&r, &s);
    return sylves_matrix4x4_multiply(&t, &temp);
}

SylvesMatrix4x4 sylves_matrix4x4_multiply(const SylvesMatrix4x4* a, const SylvesMatrix4x4* b) {
    SylvesMatrix4x4 result;
    
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            double sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum += M(a, row, k) * M(b, k, col);
            }
            M_SET(&result, row, col, sum);
        }
    }
    
    return result;
}

SylvesVector3 sylves_matrix4x4_multiply_point(const SylvesMatrix4x4* m, SylvesVector3 point) {
    double x = m->m[0] * point.x + m->m[4] * point.y + m->m[8] * point.z + m->m[12];
    double y = m->m[1] * point.x + m->m[5] * point.y + m->m[9] * point.z + m->m[13];
    double z = m->m[2] * point.x + m->m[6] * point.y + m->m[10] * point.z + m->m[14];
    double w = m->m[3] * point.x + m->m[7] * point.y + m->m[11] * point.z + m->m[15];
    
    // Perspective divide
    if (fabs(w) > 1e-6) {
        return sylves_vector3_create(x / w, y / w, z / w);
    }
    
    return sylves_vector3_create(x, y, z);
}

SylvesVector3 sylves_matrix4x4_multiply_vector(const SylvesMatrix4x4* m, SylvesVector3 vector) {
    double x = m->m[0] * vector.x + m->m[4] * vector.y + m->m[8] * vector.z;
    double y = m->m[1] * vector.x + m->m[5] * vector.y + m->m[9] * vector.z;
    double z = m->m[2] * vector.x + m->m[6] * vector.y + m->m[10] * vector.z;
    
    return sylves_vector3_create(x, y, z);
}

bool sylves_matrix4x4_invert(const SylvesMatrix4x4* m, SylvesMatrix4x4* result) {
    double inv[16], det;
    const double* src = m->m;
    
    // Calculate the inverse using cofactors
    inv[0] = src[5]  * src[10] * src[15] - 
             src[5]  * src[11] * src[14] - 
             src[9]  * src[6]  * src[15] + 
             src[9]  * src[7]  * src[14] +
             src[13] * src[6]  * src[11] - 
             src[13] * src[7]  * src[10];

    inv[4] = -src[4]  * src[10] * src[15] + 
              src[4]  * src[11] * src[14] + 
              src[8]  * src[6]  * src[15] - 
              src[8]  * src[7]  * src[14] - 
              src[12] * src[6]  * src[11] + 
              src[12] * src[7]  * src[10];

    inv[8] = src[4]  * src[9] * src[15] - 
             src[4]  * src[11] * src[13] - 
             src[8]  * src[5] * src[15] + 
             src[8]  * src[7] * src[13] + 
             src[12] * src[5] * src[11] - 
             src[12] * src[7] * src[9];

    inv[12] = -src[4]  * src[9] * src[14] + 
               src[4]  * src[10] * src[13] +
               src[8]  * src[5] * src[14] - 
               src[8]  * src[6] * src[13] - 
               src[12] * src[5] * src[10] + 
               src[12] * src[6] * src[9];

    inv[1] = -src[1]  * src[10] * src[15] + 
              src[1]  * src[11] * src[14] + 
              src[9]  * src[2] * src[15] - 
              src[9]  * src[3] * src[14] - 
              src[13] * src[2] * src[11] + 
              src[13] * src[3] * src[10];

    inv[5] = src[0]  * src[10] * src[15] - 
             src[0]  * src[11] * src[14] - 
             src[8]  * src[2] * src[15] + 
             src[8]  * src[3] * src[14] + 
             src[12] * src[2] * src[11] - 
             src[12] * src[3] * src[10];

    inv[9] = -src[0]  * src[9] * src[15] + 
              src[0]  * src[11] * src[13] + 
              src[8]  * src[1] * src[15] - 
              src[8]  * src[3] * src[13] - 
              src[12] * src[1] * src[11] + 
              src[12] * src[3] * src[9];

    inv[13] = src[0]  * src[9] * src[14] - 
              src[0]  * src[10] * src[13] - 
              src[8]  * src[1] * src[14] + 
              src[8]  * src[2] * src[13] + 
              src[12] * src[1] * src[10] - 
              src[12] * src[2] * src[9];

    inv[2] = src[1]  * src[6] * src[15] - 
             src[1]  * src[7] * src[14] - 
             src[5]  * src[2] * src[15] + 
             src[5]  * src[3] * src[14] + 
             src[13] * src[2] * src[7] - 
             src[13] * src[3] * src[6];

    inv[6] = -src[0]  * src[6] * src[15] + 
              src[0]  * src[7] * src[14] + 
              src[4]  * src[2] * src[15] - 
              src[4]  * src[3] * src[14] - 
              src[12] * src[2] * src[7] + 
              src[12] * src[3] * src[6];

    inv[10] = src[0]  * src[5] * src[15] - 
              src[0]  * src[7] * src[13] - 
              src[4]  * src[1] * src[15] + 
              src[4]  * src[3] * src[13] + 
              src[12] * src[1] * src[7] - 
              src[12] * src[3] * src[5];

    inv[14] = -src[0]  * src[5] * src[14] + 
               src[0]  * src[6] * src[13] + 
               src[4]  * src[1] * src[14] - 
               src[4]  * src[2] * src[13] - 
               src[12] * src[1] * src[6] + 
               src[12] * src[2] * src[5];

    inv[3] = -src[1] * src[6] * src[11] + 
              src[1] * src[7] * src[10] + 
              src[5] * src[2] * src[11] - 
              src[5] * src[3] * src[10] - 
              src[9] * src[2] * src[7] + 
              src[9] * src[3] * src[6];

    inv[7] = src[0] * src[6] * src[11] - 
             src[0] * src[7] * src[10] - 
             src[4] * src[2] * src[11] + 
             src[4] * src[3] * src[10] + 
             src[8] * src[2] * src[7] - 
             src[8] * src[3] * src[6];

    inv[11] = -src[0] * src[5] * src[11] + 
               src[0] * src[7] * src[9] + 
               src[4] * src[1] * src[11] - 
               src[4] * src[3] * src[9] - 
               src[8] * src[1] * src[7] + 
               src[8] * src[3] * src[5];

    inv[15] = src[0] * src[5] * src[10] - 
              src[0] * src[6] * src[9] - 
              src[4] * src[1] * src[10] + 
              src[4] * src[2] * src[9] + 
              src[8] * src[1] * src[6] - 
              src[8] * src[2] * src[5];

    det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];

    if (fabs(det) < 1e-6) {
        return false;
    }

    det = 1.0 / det;

    for (int i = 0; i < 16; i++) {
        result->m[i] = inv[i] * det;
    }

    return true;
}

SylvesMatrix4x4 sylves_matrix4x4_transpose(const SylvesMatrix4x4* m) {
    SylvesMatrix4x4 result;
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            M_SET(&result, row, col, M(m, col, row));
        }
    }
    
    return result;
}

SylvesMatrix4x4 sylves_matrix4x4_look_at(SylvesVector3 eye, SylvesVector3 target, SylvesVector3 up) {
    SylvesVector3 f = sylves_vector3_normalize(sylves_vector3_subtract(target, eye));
    SylvesVector3 s = sylves_vector3_normalize(sylves_vector3_cross(f, up));
    SylvesVector3 u = sylves_vector3_cross(s, f);
    
    SylvesMatrix4x4 result = sylves_matrix4x4_identity();
    
    result.m[0] = s.x;
    result.m[4] = s.y;
    result.m[8] = s.z;
    
    result.m[1] = u.x;
    result.m[5] = u.y;
    result.m[9] = u.z;
    
    result.m[2] = -f.x;
    result.m[6] = -f.y;
    result.m[10] = -f.z;
    
    result.m[12] = -sylves_vector3_dot(s, eye);
    result.m[13] = -sylves_vector3_dot(u, eye);
    result.m[14] = sylves_vector3_dot(f, eye);
    
    return result;
}

SylvesMatrix4x4 sylves_matrix4x4_perspective(double fov_radians, double aspect, double near_plane, double far_plane) {
    SylvesMatrix4x4 result = sylves_matrix4x4_zero();
    
    double tan_half_fov = tan(fov_radians / 2.0);
    
    result.m[0] = 1.0 / (aspect * tan_half_fov);
    result.m[5] = 1.0 / tan_half_fov;
    result.m[10] = -(far_plane + near_plane) / (far_plane - near_plane);
    result.m[11] = -1.0;
    result.m[14] = -(2.0 * far_plane * near_plane) / (far_plane - near_plane);
    
    return result;
}

SylvesMatrix4x4 sylves_matrix4x4_orthographic(double left, double right, double bottom, double top, double near_plane, double far_plane) {
    SylvesMatrix4x4 result = sylves_matrix4x4_identity();
    
    result.m[0] = 2.0 / (right - left);
    result.m[5] = 2.0 / (top - bottom);
    result.m[10] = -2.0 / (far_plane - near_plane);
    
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(far_plane + near_plane) / (far_plane - near_plane);
    
    return result;
}
