/**
 * @file triangle_interpolation.h
 * @brief Triangle interpolation methods for deformations
 */

#ifndef SYLVES_TRIANGLE_INTERPOLATION_H
#define SYLVES_TRIANGLE_INTERPOLATION_H

#include "types.h"
#include "mesh.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Triangle interpolation context
 * 
 * Contains vertices for triangular interpolation.
 * Convention: XY plane with equilateral triangle of side 1, vertices:
 *  (0.5f, -0.5f / sqrt(3))
 *  (0, 1 / sqrt(3))
 *  (-0.5f, -0.5f / sqrt(3))
 */
typedef struct SylvesTriangleInterpolation {
    /* For 2D interpolation (3 vertices) */
    SylvesVector3 v1, v2, v3;
    
    /* For 3D prism interpolation (6 vertices) */
    SylvesVector3 v4, v5, v6;
    bool is_3d;
} SylvesTriangleInterpolation;

/**
 * @brief Create triangle interpolation from mesh face
 * 
 * @param mesh Source mesh data
 * @param submesh Submesh index
 * @param face Face index
 * @param invert_winding Whether to invert winding order
 * @param error_out Optional error output
 * @return New triangle interpolation context
 */
SylvesTriangleInterpolation* sylves_triangle_interpolation_create_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create triangle prism interpolation from mesh face with offsets
 * 
 * @param mesh Source mesh data
 * @param submesh Submesh index
 * @param face Face index
 * @param invert_winding Whether to invert winding order
 * @param mesh_offset1 First offset along normals
 * @param mesh_offset2 Second offset along normals
 * @param error_out Optional error output
 * @return New triangle interpolation context
 */
SylvesTriangleInterpolation* sylves_triangle_interpolation_create_prism_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    float mesh_offset1,
    float mesh_offset2,
    SylvesError* error_out
);

/**
 * @brief Create triangle interpolation from 3 vertices
 */
SylvesTriangleInterpolation* sylves_triangle_interpolation_create_2d(
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    const SylvesVector3* v3,
    SylvesError* error_out
);

/**
 * @brief Create triangle prism interpolation from 6 vertices
 */
SylvesTriangleInterpolation* sylves_triangle_interpolation_create_3d(
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    const SylvesVector3* v3,
    const SylvesVector3* v4,
    const SylvesVector3* v5,
    const SylvesVector3* v6,
    SylvesError* error_out
);

/**
 * @brief Destroy triangle interpolation
 */
void sylves_triangle_interpolation_destroy(SylvesTriangleInterpolation* interp);

/**
 * @brief Interpolate position
 * 
 * For 2D: Linear interpolation on triangle, z value of p is unused
 * For 3D: Linear interpolation on prism
 */
SylvesVector3 sylves_triangle_interpolation_position(
    const SylvesTriangleInterpolation* interp,
    SylvesVector3 p
);

/**
 * @brief Get Jacobian matrix for position interpolation
 */
void sylves_triangle_interpolation_jacobi(
    const SylvesTriangleInterpolation* interp,
    SylvesVector3 p,
    SylvesMatrix4x4* jacobi
);

/**
 * @brief Compute barycentric coordinates for standard equilateral triangle
 * 
 * @param p Point in XY plane
 * @return Barycentric coordinates (x=b1, y=b2, z=b3)
 */
SylvesVector3 sylves_triangle_std_barycentric(SylvesVector2 p);

/**
 * @brief Compute barycentric coordinate derivatives
 * 
 * @param dbdx Output derivative with respect to x
 * @param dbdy Output derivative with respect to y
 */
void sylves_triangle_std_barycentric_diff(SylvesVector3* dbdx, SylvesVector3* dbdy);

/**
 * @brief Interpolation context for mesh attributes
 */
typedef struct SylvesTriangleAttributeInterp {
    /* Attribute values at vertices */
    union {
        SylvesVector2 v2[6];
        SylvesVector3 v3[6];
        SylvesVector4 v4[6];
    } values;
    
    bool is_3d;
    int dimensions; /* 2, 3, or 4 */
} SylvesTriangleAttributeInterp;

/**
 * @brief Create attribute interpolation for normals
 */
SylvesTriangleAttributeInterp* sylves_triangle_interp_normals_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create attribute interpolation for tangents
 */
SylvesTriangleAttributeInterp* sylves_triangle_interp_tangents_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create attribute interpolation for UVs
 */
SylvesTriangleAttributeInterp* sylves_triangle_interp_uvs_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Destroy attribute interpolation
 */
void sylves_triangle_attribute_interp_destroy(SylvesTriangleAttributeInterp* interp);

/**
 * @brief Interpolate vector2 attribute
 */
SylvesVector2 sylves_triangle_interp_vector2(
    const SylvesTriangleAttributeInterp* interp,
    SylvesVector3 p
);

/**
 * @brief Interpolate vector3 attribute
 */
SylvesVector3 sylves_triangle_interp_vector3(
    const SylvesTriangleAttributeInterp* interp,
    SylvesVector3 p
);

/**
 * @brief Interpolate vector4 attribute
 */
SylvesVector4 sylves_triangle_interp_vector4(
    const SylvesTriangleAttributeInterp* interp,
    SylvesVector3 p
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_TRIANGLE_INTERPOLATION_H */
