/**
 * @file trs.h
 * @brief Transform-Rotation-Scale operations
 */

#ifndef SYLVES_TRS_H
#define SYLVES_TRS_H

#include "types.h"
#include "vector.h"
#include "matrix.h"
#include <stdbool.h>


/* Forward declarations */
typedef struct SylvesQuaternion SylvesQuaternion;

/* TRS creation */
SylvesTRS sylves_trs_create(SylvesVector3 position, SylvesQuaternion rotation, SylvesVector3 scale);
SylvesTRS sylves_trs_create_from_matrix(SylvesVector3 position, SylvesMatrix4x4 rotation, SylvesVector3 scale);
SylvesTRS sylves_trs_identity(void);
SylvesTRS sylves_trs_from_position(SylvesVector3 position);
SylvesTRS sylves_trs_from_rotation(SylvesQuaternion rotation);
SylvesTRS sylves_trs_from_scale(SylvesVector3 scale);

/* TRS conversion */
SylvesMatrix4x4 sylves_trs_to_matrix(SylvesTRS trs);

/* TRS transformation */
SylvesVector3 sylves_trs_transform_point(SylvesTRS trs, SylvesVector3 point);
SylvesVector3 sylves_trs_transform_vector(SylvesTRS trs, SylvesVector3 vector);
SylvesVector3 sylves_trs_transform_direction(SylvesTRS trs, SylvesVector3 direction);

/* TRS operations */
SylvesTRS sylves_trs_inverse(SylvesTRS trs);
SylvesTRS sylves_trs_combine(SylvesTRS a, SylvesTRS b);
SylvesTRS sylves_trs_lerp(SylvesTRS a, SylvesTRS b, double t);

/* TRS comparison */
bool sylves_trs_approx_equal(SylvesTRS a, SylvesTRS b, double epsilon);


#endif /* SYLVES_TRS_H */
