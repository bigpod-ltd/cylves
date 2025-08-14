/**
 * @file matrix.h
 * @brief Matrix operations
 */

#ifndef SYLVES_MATRIX_H
#define SYLVES_MATRIX_H

#include "types.h"
#include "vector.h"
#include <stdbool.h>


/* Forward declarations */
typedef struct SylvesQuaternion SylvesQuaternion;

/* Matrix creation */
SylvesMatrix4x4 sylves_matrix4x4_identity(void);
SylvesMatrix4x4 sylves_matrix4x4_zero(void);
SylvesMatrix4x4 sylves_matrix4x4_translation(SylvesVector3 translation);
SylvesMatrix4x4 sylves_matrix4x4_scale(SylvesVector3 scale);
SylvesMatrix4x4 sylves_matrix4x4_rotation_x(double angle_radians);
SylvesMatrix4x4 sylves_matrix4x4_rotation_y(double angle_radians);
SylvesMatrix4x4 sylves_matrix4x4_rotation_z(double angle_radians);
SylvesMatrix4x4 sylves_matrix4x4_from_quaternion(SylvesQuaternion q);
SylvesMatrix4x4 sylves_matrix4x4_from_trs(SylvesTRS trs);
SylvesMatrix4x4 sylves_matrix4x4_look_at(SylvesVector3 eye, SylvesVector3 target, SylvesVector3 up);
SylvesMatrix4x4 sylves_matrix4x4_perspective(double fov_radians, double aspect, double near_plane, double far_plane);
SylvesMatrix4x4 sylves_matrix4x4_orthographic(double left, double right, double bottom, double top, double near_plane, double far_plane);

/* Matrix operations */
SylvesMatrix4x4 sylves_matrix4x4_multiply(const SylvesMatrix4x4* a, const SylvesMatrix4x4* b);
SylvesVector3 sylves_matrix4x4_multiply_point(const SylvesMatrix4x4* m, SylvesVector3 point);
SylvesVector3 sylves_matrix4x4_multiply_vector(const SylvesMatrix4x4* m, SylvesVector3 vector);
bool sylves_matrix4x4_invert(const SylvesMatrix4x4* m, SylvesMatrix4x4* result);
SylvesMatrix4x4 sylves_matrix4x4_transpose(const SylvesMatrix4x4* m);


#endif /* SYLVES_MATRIX_H */
