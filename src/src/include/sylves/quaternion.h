/**
 * @file quaternion.h
 * @brief Quaternion mathematics for rotations
 */

#ifndef SYLVES_QUATERNION_H
#define SYLVES_QUATERNION_H

#include "types.h"
#include "vector.h"
#include <math.h>


/**
 * @brief Quaternion for representing rotations
 */
typedef struct SylvesQuaternion {
    double x;  /**< X component (imaginary) */
    double y;  /**< Y component (imaginary) */
    double z;  /**< Z component (imaginary) */
    double w;  /**< W component (real) */
} SylvesQuaternion;

/* Quaternion creation */

/**
 * @brief Create a quaternion from components
 */
static inline SylvesQuaternion sylves_quaternion_create(double x, double y, double z, double w) {
    SylvesQuaternion q = {x, y, z, w};
    return q;
}

/**
 * @brief Create identity quaternion (no rotation)
 */
static inline SylvesQuaternion sylves_quaternion_identity(void) {
    return sylves_quaternion_create(0.0, 0.0, 0.0, 1.0);
}

/**
 * @brief Create quaternion from axis and angle
 */
static inline SylvesQuaternion sylves_quaternion_from_axis_angle(SylvesVector3 axis, double angle) {
    double half_angle = angle * 0.5;
    double s = sin(half_angle);
    SylvesVector3 normalized = sylves_vector3_normalize(axis);
    return sylves_quaternion_create(
        normalized.x * s,
        normalized.y * s,
        normalized.z * s,
        cos(half_angle)
    );
}

/**
 * @brief Create quaternion from Euler angles (roll, pitch, yaw)
 */
static inline SylvesQuaternion sylves_quaternion_from_euler(double roll, double pitch, double yaw) {
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);
    
    return sylves_quaternion_create(
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
    );
}

/* Quaternion operations */

/**
 * @brief Add two quaternions
 */
static inline SylvesQuaternion sylves_quaternion_add(SylvesQuaternion a, SylvesQuaternion b) {
    return sylves_quaternion_create(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

/**
 * @brief Multiply quaternion by scalar
 */
static inline SylvesQuaternion sylves_quaternion_scale(SylvesQuaternion q, double s) {
    return sylves_quaternion_create(q.x * s, q.y * s, q.z * s, q.w * s);
}

/**
 * @brief Dot product of two quaternions
 */
static inline double sylves_quaternion_dot(SylvesQuaternion a, SylvesQuaternion b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/**
 * @brief Quaternion magnitude squared
 */
static inline double sylves_quaternion_length_squared(SylvesQuaternion q) {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

/**
 * @brief Quaternion magnitude
 */
static inline double sylves_quaternion_length(SylvesQuaternion q) {
    return sqrt(sylves_quaternion_length_squared(q));
}

/**
 * @brief Normalize quaternion
 */
static inline SylvesQuaternion sylves_quaternion_normalize(SylvesQuaternion q) {
    double len = sylves_quaternion_length(q);
    if (len > 0.0) {
        double inv_len = 1.0 / len;
        return sylves_quaternion_scale(q, inv_len);
    }
    return q;
}

/**
 * @brief Conjugate of a quaternion
 */
static inline SylvesQuaternion sylves_quaternion_conjugate(SylvesQuaternion q) {
    return sylves_quaternion_create(-q.x, -q.y, -q.z, q.w);
}

/**
 * @brief Inverse of a quaternion
 */
static inline SylvesQuaternion sylves_quaternion_inverse(SylvesQuaternion q) {
    double len_sq = sylves_quaternion_length_squared(q);
    if (len_sq > 0.0) {
        SylvesQuaternion conj = sylves_quaternion_conjugate(q);
        return sylves_quaternion_scale(conj, 1.0 / len_sq);
    }
    return q;
}

/**
 * @brief Multiply two quaternions (q1 * q2)
 */
static inline SylvesQuaternion sylves_quaternion_multiply(SylvesQuaternion a, SylvesQuaternion b) {
    return sylves_quaternion_create(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

/**
 * @brief Rotate a vector by a quaternion
 */
static inline SylvesVector3 sylves_quaternion_rotate_vector(SylvesQuaternion q, SylvesVector3 v) {
    // Pure quaternion from vector
    SylvesQuaternion qv = sylves_quaternion_create(v.x, v.y, v.z, 0.0);
    
    // q * qv * q^-1
    SylvesQuaternion qconj = sylves_quaternion_conjugate(q);
    SylvesQuaternion result = sylves_quaternion_multiply(
        sylves_quaternion_multiply(q, qv), 
        qconj
    );
    
    return sylves_vector3_create(result.x, result.y, result.z);
}

/**
 * @brief Spherical linear interpolation between two quaternions
 */
static inline SylvesQuaternion sylves_quaternion_slerp(SylvesQuaternion a, SylvesQuaternion b, double t) {
    // Normalize inputs
    a = sylves_quaternion_normalize(a);
    b = sylves_quaternion_normalize(b);
    
    double dot = sylves_quaternion_dot(a, b);
    
    // If the dot product is negative, negate one quaternion to take the shorter path
    if (dot < 0.0) {
        b = sylves_quaternion_scale(b, -1.0);
        dot = -dot;
    }
    
    // If quaternions are nearly identical, use linear interpolation
    if (dot > 0.9995) {
        SylvesQuaternion result = sylves_quaternion_add(
            sylves_quaternion_scale(a, 1.0 - t),
            sylves_quaternion_scale(b, t)
        );
        return sylves_quaternion_normalize(result);
    }
    
    // Calculate angle between quaternions
    double theta = acos(fmin(fmax(dot, -1.0), 1.0));
    double sin_theta = sin(theta);
    
    if (fabs(sin_theta) < 0.001) {
        // Fall back to linear interpolation for small angles
        return sylves_quaternion_add(
            sylves_quaternion_scale(a, 0.5),
            sylves_quaternion_scale(b, 0.5)
        );
    }
    
    double factor_a = sin((1.0 - t) * theta) / sin_theta;
    double factor_b = sin(t * theta) / sin_theta;
    
    return sylves_quaternion_add(
        sylves_quaternion_scale(a, factor_a),
        sylves_quaternion_scale(b, factor_b)
    );
}

/**
 * @brief Convert quaternion to axis-angle representation
 */
static inline void sylves_quaternion_to_axis_angle(SylvesQuaternion q, SylvesVector3* axis, double* angle) {
    q = sylves_quaternion_normalize(q);
    
    *angle = 2.0 * acos(fmin(fmax(q.w, -1.0), 1.0));
    
    double s = sqrt(1.0 - q.w * q.w);
    if (s < 0.001) {
        // If s is close to zero, direction doesn't matter
        *axis = sylves_vector3_create(1.0, 0.0, 0.0);
    } else {
        *axis = sylves_vector3_create(q.x / s, q.y / s, q.z / s);
    }
}

/**
 * @brief Convert quaternion to Euler angles (roll, pitch, yaw)
 */
static inline void sylves_quaternion_to_euler(SylvesQuaternion q, double* roll, double* pitch, double* yaw) {
    // Roll (x-axis rotation)
    double sinr_cosp = 2.0 * (q.w * q.x + q.y * q.z);
    double cosr_cosp = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    *roll = atan2(sinr_cosp, cosr_cosp);
    
    // Pitch (y-axis rotation)
    double sinp = 2.0 * (q.w * q.y - q.z * q.x);
    if (fabs(sinp) >= 1.0) {
        *pitch = copysign(M_PI / 2.0, sinp); // Use 90 degrees if out of range
    } else {
        *pitch = asin(sinp);
    }
    
    // Yaw (z-axis rotation)
    double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    *yaw = atan2(siny_cosp, cosy_cosp);
}

/**
 * @brief Check if quaternions are approximately equal
 */
static inline bool sylves_quaternion_approx_equal(SylvesQuaternion a, SylvesQuaternion b, double epsilon) {
    return fabs(a.x - b.x) < epsilon &&
           fabs(a.y - b.y) < epsilon &&
           fabs(a.z - b.z) < epsilon &&
           fabs(a.w - b.w) < epsilon;
}


#endif /* SYLVES_QUATERNION_H */
