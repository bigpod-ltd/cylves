/**
 * @file quad_interpolation.h
 * @brief Quad interpolation methods for deformations
 */

#ifndef SYLVES_QUAD_INTERPOLATION_H
#define SYLVES_QUAD_INTERPOLATION_H

#include "types.h"
#include "mesh.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Quad interpolation context
 * 
 * Contains vertices for quadrilateral interpolation.
 * Convention: XY plane with unit square
 */
typedef struct SylvesQuadInterpolation {
    /* For 2D interpolation (4 vertices) */
    SylvesVector3 v1, v2, v3, v4;
    
    /* For 3D prism interpolation (8 vertices) */
    SylvesVector3 v5, v6, v7, v8;
    bool is_3d;
} SylvesQuadInterpolation;

/**
 * @brief Create quad interpolation from mesh face
 * 
 * @param mesh Source mesh data
 * @param submesh Submesh index
 * @param face Face index
 * @param invert_winding Whether to invert winding order
 * @param error_out Optional error output
 * @return New quad interpolation context
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create quad prism interpolation from mesh face with offsets
 * 
 * @param mesh Source mesh data
 * @param submesh Submesh index
 * @param face Face index
 * @param invert_winding Whether to invert winding order
 * @param mesh_offset1 First offset along normals
 * @param mesh_offset2 Second offset along normals
 * @param error_out Optional error output
 * @return New quad interpolation context
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_prism_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    float mesh_offset1,
    float mesh_offset2,
    SylvesError* error_out
);

/**
 * @brief Create quad interpolation from 4 vertices
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_2d(
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    const SylvesVector3* v3,
    const SylvesVector3* v4,
    SylvesError* error_out
);

/**
 * @brief Create quad prism interpolation from 8 vertices
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_3d(
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    const SylvesVector3* v3,
    const SylvesVector3* v4,
    const SylvesVector3* v5,
    const SylvesVector3* v6,
    const SylvesVector3* v7,
    const SylvesVector3* v8,
    SylvesError* error_out
);

/**
 * @brief Destroy quad interpolation
 */
void sylves_quad_interpolation_destroy(SylvesQuadInterpolation* interp);

/**
 * @brief Interpolate position
 * 
 * For 2D: Bilinear interpolation on quad, z value of p is unused
 * For 3D: Trilinear interpolation on prism
 */
SylvesVector3 sylves_quad_interpolation_position(
    const SylvesQuadInterpolation* interp,
    SylvesVector3 p
);

/**
 * @brief Get Jacobian matrix for position interpolation
 */
void sylves_quad_interpolation_jacobi(
    const SylvesQuadInterpolation* interp,
    SylvesVector3 p,
    SylvesMatrix4x4* jacobi
);

/**
 * @brief Interpolation context for mesh attributes
 */
typedef struct SylvesQuadAttributeInterp {
    /* Attribute values at vertices */
    union {
        SylvesVector2 v2[8];
        SylvesVector3 v3[8];
        SylvesVector4 v4[8];
    } values;
    
    bool is_3d;
    int dimensions; /* 2, 3, or 4 */
} SylvesQuadAttributeInterp;

/**
 * @brief Create attribute interpolation for normals
 */
SylvesQuadAttributeInterp* sylves_quad_interp_normals_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create attribute interpolation for tangents
 */
SylvesQuadAttributeInterp* sylves_quad_interp_tangents_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create attribute interpolation for UVs
 */
SylvesQuadAttributeInterp* sylves_quad_interp_uvs_create(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Destroy attribute interpolation
 */
void sylves_quad_attribute_interp_destroy(SylvesQuadAttributeInterp* interp);

/**
 * @brief Interpolate vector2 attribute
 */
SylvesVector2 sylves_quad_interp_vector2(
    const SylvesQuadAttributeInterp* interp,
    SylvesVector3 p
);

/**
 * @brief Interpolate vector3 attribute
 */
SylvesVector3 sylves_quad_interp_vector3(
    const SylvesQuadAttributeInterp* interp,
    SylvesVector3 p
);

/**
 * @brief Interpolate vector4 attribute
 */
SylvesVector4 sylves_quad_interp_vector4(
    const SylvesQuadAttributeInterp* interp,
    SylvesVector3 p
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_QUAD_INTERPOLATION_H */
