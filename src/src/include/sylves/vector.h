/**
 * @file vector.h
 * @brief Vector mathematics operations
 */

#ifndef SYLVES_VECTOR_H
#define SYLVES_VECTOR_H

#include "types.h"
#include <math.h>


/* Vector3 operations */

/**
 * @brief Create a 3D vector
 */
static inline SylvesVector3 sylves_vector3_create(double x, double y, double z) {
    SylvesVector3 v = {x, y, z};
    return v;
}

/**
 * @brief Zero vector
 */
static inline SylvesVector3 sylves_vector3_zero(void) {
    return sylves_vector3_create(0.0, 0.0, 0.0);
}

/**
 * @brief Unit X vector
 */
static inline SylvesVector3 sylves_vector3_unit_x(void) {
    return sylves_vector3_create(1.0, 0.0, 0.0);
}

/**
 * @brief Unit Y vector
 */
static inline SylvesVector3 sylves_vector3_unit_y(void) {
    return sylves_vector3_create(0.0, 1.0, 0.0);
}

/**
 * @brief Unit Z vector
 */
static inline SylvesVector3 sylves_vector3_unit_z(void) {
    return sylves_vector3_create(0.0, 0.0, 1.0);
}

/**
 * @brief Add two vectors
 */
static inline SylvesVector3 sylves_vector3_add(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

/**
 * @brief Subtract two vectors
 */
static inline SylvesVector3 sylves_vector3_subtract(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

/**
 * @brief Multiply vector by scalar
 */
static inline SylvesVector3 sylves_vector3_scale(SylvesVector3 v, double s) {
    return sylves_vector3_create(v.x * s, v.y * s, v.z * s);
}

/**
 * @brief Component-wise multiplication
 */
static inline SylvesVector3 sylves_vector3_multiply(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(a.x * b.x, a.y * b.y, a.z * b.z);
}

/**
 * @brief Dot product
 */
static inline double sylves_vector3_dot(SylvesVector3 a, SylvesVector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief Cross product
 */
static inline SylvesVector3 sylves_vector3_cross(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

/**
 * @brief Vector magnitude squared
 */
static inline double sylves_vector3_length_squared(SylvesVector3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
 * @brief Vector magnitude
 */
static inline double sylves_vector3_length(SylvesVector3 v) {
    return sqrt(sylves_vector3_length_squared(v));
}

/**
 * @brief Normalize vector
 */
static inline SylvesVector3 sylves_vector3_normalize(SylvesVector3 v) {
    double len = sylves_vector3_length(v);
    if (len > 0.0) {
        return sylves_vector3_scale(v, 1.0 / len);
    }
    return v;
}

/**
 * @brief Distance between two points
 */
static inline double sylves_vector3_distance(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_length(sylves_vector3_subtract(b, a));
}

/**
 * @brief Linear interpolation
 */
static inline SylvesVector3 sylves_vector3_lerp(SylvesVector3 a, SylvesVector3 b, double t) {
    return sylves_vector3_create(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

/**
 * @brief Component-wise minimum
 */
static inline SylvesVector3 sylves_vector3_min(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(
        fmin(a.x, b.x),
        fmin(a.y, b.y),
        fmin(a.z, b.z)
    );
}

/**
 * @brief Component-wise maximum
 */
static inline SylvesVector3 sylves_vector3_max(SylvesVector3 a, SylvesVector3 b) {
    return sylves_vector3_create(
        fmax(a.x, b.x),
        fmax(a.y, b.y),
        fmax(a.z, b.z)
    );
}

/**
 * @brief Check if vectors are approximately equal
 */
static inline bool sylves_vector3_approx_equal(SylvesVector3 a, SylvesVector3 b, double epsilon) {
    return fabs(a.x - b.x) < epsilon &&
           fabs(a.y - b.y) < epsilon &&
           fabs(a.z - b.z) < epsilon;
}

/* Vector3Int operations */

/**
 * @brief Create a 3D integer vector
 */
static inline SylvesVector3Int sylves_vector3int_create(int x, int y, int z) {
    SylvesVector3Int v = {x, y, z};
    return v;
}

/**
 * @brief Convert integer vector to double vector
 */
static inline SylvesVector3 sylves_vector3int_to_vector3(SylvesVector3Int v) {
    return sylves_vector3_create((double)v.x, (double)v.y, (double)v.z);
}

/**
 * @brief Add integer vectors
 */
static inline SylvesVector3Int sylves_vector3int_add(SylvesVector3Int a, SylvesVector3Int b) {
    return sylves_vector3int_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

/**
 * @brief Subtract integer vectors
 */
static inline SylvesVector3Int sylves_vector3int_subtract(SylvesVector3Int a, SylvesVector3Int b) {
    return sylves_vector3int_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

/* Vector2 operations */

/**
 * @brief Create a 2D vector
 */
static inline SylvesVector2 sylves_vector2_create(double x, double y) {
    SylvesVector2 v = {x, y};
    return v;
}

/**
 * @brief Add 2D vectors
 */
static inline SylvesVector2 sylves_vector2_add(SylvesVector2 a, SylvesVector2 b) {
    return sylves_vector2_create(a.x + b.x, a.y + b.y);
}

/**
 * @brief Subtract 2D vectors
 */
static inline SylvesVector2 sylves_vector2_subtract(SylvesVector2 a, SylvesVector2 b) {
    return sylves_vector2_create(a.x - b.x, a.y - b.y);
}

/**
 * @brief Scale 2D vector
 */
static inline SylvesVector2 sylves_vector2_scale(SylvesVector2 v, double s) {
    return sylves_vector2_create(v.x * s, v.y * s);
}

/**
 * @brief 2D dot product
 */
static inline double sylves_vector2_dot(SylvesVector2 a, SylvesVector2 b) {
    return a.x * b.x + a.y * b.y;
}

/**
 * @brief 2D vector length
 */
static inline double sylves_vector2_length(SylvesVector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

/**
 * @brief Normalize 2D vector
 */
static inline SylvesVector2 sylves_vector2_normalize(SylvesVector2 v) {
    double len = sylves_vector2_length(v);
    if (len > 0.0) {
        return sylves_vector2_scale(v, 1.0 / len);
    }
    return v;
}


#endif /* SYLVES_VECTOR_H */
