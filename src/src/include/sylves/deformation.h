/**
 * @file deformation.h
 * @brief Deformation interface for continuous, differentiable mappings between spaces
 */

#ifndef SYLVES_DEFORMATION_H
#define SYLVES_DEFORMATION_H

#include "types.h"
#include "mesh.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function type for deforming a point
 */
typedef SylvesVector3 (*SylvesDeformPointFunc)(SylvesVector3 p, void* context);

/**
 * @brief Function type for deforming a normal vector
 */
typedef SylvesVector3 (*SylvesDeformNormalFunc)(SylvesVector3 p, SylvesVector3 n, void* context);

/**
 * @brief Function type for deforming a tangent vector
 */
typedef SylvesVector4 (*SylvesDeformTangentFunc)(SylvesVector3 p, SylvesVector4 t, void* context);

/**
 * @brief Function type for getting Jacobian matrix at a point
 */
typedef void (*SylvesGetJacobiFunc)(SylvesVector3 p, SylvesMatrix4x4* jacobi, void* context);

/**
 * @brief Deformation structure
 * 
 * A deformation is a continuous, differentiable mapping from one space to another.
 * It is used to warp meshes in arbitrary ways by mapping the vertices, normals and tangents.
 */
struct SylvesDeformation {
    /* Function pointers */
    SylvesDeformPointFunc deform_point;
    SylvesDeformNormalFunc deform_normal;
    SylvesDeformTangentFunc deform_tangent;
    SylvesGetJacobiFunc get_jacobi;
    
    /* Context for function callbacks */
    void* context;
    
    /* Winding flags */
    bool invert_winding;          /** < Cached final invert winding flag > */
    bool inner_invert_winding;    /** < User-requested invert winding before transform parity > */
    
    /* Numerical differentiation step */
    float epsilon;
    
    /* Pre/post transformation matrices */
    SylvesMatrix4x4 pre_deform;
    SylvesMatrix4x4 post_deform;
    SylvesMatrix4x4 pre_deform_it;   /**< Inverse transpose of pre_deform */
    SylvesMatrix4x4 post_deform_it;  /**< Inverse transpose of post_deform */
};

/**
 * @brief Create a deformation with all functions specified
 */
SylvesDeformation* sylves_deformation_create(
    SylvesDeformPointFunc deform_point,
    SylvesDeformNormalFunc deform_normal,
    SylvesDeformTangentFunc deform_tangent,
    SylvesGetJacobiFunc get_jacobi,
    void* context,
    bool invert_winding,
    SylvesError* error_out
);

/**
 * @brief Create a deformation with point and jacobi functions
 * Normal and tangent functions will be derived from the Jacobian.
 */
SylvesDeformation* sylves_deformation_create_with_jacobi(
    SylvesDeformPointFunc deform_point,
    SylvesGetJacobiFunc get_jacobi,
    bool invert_winding,
    void* context,
    SylvesError* error_out
);

/**
 * @brief Create a deformation from point function with numerical differentiation
 * @param step Step size for numerical differentiation (default 1e-3)
 */
SylvesDeformation* sylves_deformation_create_numerical(
    SylvesDeformPointFunc deform_point,
    float step,
    bool invert_winding,
    void* context,
    SylvesError* error_out
);

/**
 * @brief Get identity deformation singleton
 */
SylvesDeformation* sylves_deformation_identity(void);

/**
 * @brief Destroy deformation
 */
void sylves_deformation_destroy(SylvesDeformation* deformation);

/**
 * @brief Clone deformation
 */
SylvesDeformation* sylves_deformation_clone(const SylvesDeformation* deformation, SylvesError* error_out);

/**
 * @brief Deform a point
 */
SylvesVector3 sylves_deformation_deform_point(const SylvesDeformation* deformation, SylvesVector3 p);

/**
 * @brief Deform a normal vector
 */
SylvesVector3 sylves_deformation_deform_normal(const SylvesDeformation* deformation, SylvesVector3 p, SylvesVector3 n);

/**
 * @brief Deform a tangent vector
 */
SylvesVector4 sylves_deformation_deform_tangent(const SylvesDeformation* deformation, SylvesVector3 p, SylvesVector4 t);

/**
 * @brief Get Jacobian matrix at a point
 */
void sylves_deformation_get_jacobi(const SylvesDeformation* deformation, SylvesVector3 p, SylvesMatrix4x4* jacobi);

/**
 * @brief Check if winding should be inverted
 */
bool sylves_deformation_get_invert_winding(const SylvesDeformation* deformation);

/**
 * @brief Apply pre-transformation matrix to deformation
 * Result = deformation * matrix
 */
SylvesDeformation* sylves_deformation_pre_multiply(
    const SylvesDeformation* deformation,
    const SylvesMatrix4x4* matrix,
    SylvesError* error_out
);

/**
 * @brief Apply post-transformation matrix to deformation
 * Result = matrix * deformation
 */
SylvesDeformation* sylves_deformation_post_multiply(
    const SylvesMatrix4x4* matrix,
    const SylvesDeformation* deformation,
    SylvesError* error_out
);

/**
 * @brief Deform mesh data
 */
void sylves_deformation_deform_mesh(
    const SylvesDeformation* deformation,
    const SylvesMeshData* mesh,
    SylvesMeshData* out_mesh,
    SylvesError* error_out
);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_DEFORMATION_H */
