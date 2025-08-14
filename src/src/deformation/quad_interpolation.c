/**
 * @file quad_interpolation.c
 * @brief Quad interpolation implementation for mesh faces
 */

#include "sylves/quad_interpolation.h"
#include "sylves/types.h"
#include "sylves/mesh_data.h"
#include "sylves/vector.h"
#include "sylves/errors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief Simple lerp for vectors
 */
static SylvesVector3 lerp3(SylvesVector3 a, SylvesVector3 b, float t) {
    SylvesVector3 r = { 
        a.x + (b.x - a.x) * t, 
        a.y + (b.y - a.y) * t, 
        a.z + (b.z - a.z) * t 
    }; 
    return r;
}

/**
 * @brief Get corners for 2D quad from mesh face
 * 
 * This is a placeholder - real implementation would extract vertices from mesh
 */
static bool get_corners_2d(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesVector3* v1,
    SylvesVector3* v2,
    SylvesVector3* v3,
    SylvesVector3* v4,
    SylvesError* error_out)
{
    /* TODO: Implement proper mesh face extraction */
    (void)mesh; (void)submesh; (void)face; (void)invert_winding;
    (void)v1; (void)v2; (void)v3; (void)v4;
    
    if (error_out) *error_out = SYLVES_ERROR_NOT_IMPLEMENTED;
    return false;
}

/**
 * @brief Get corners for 3D quad prism from mesh face with offsets
 */
static bool get_corners_3d(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    float mesh_offset1,
    float mesh_offset2,
    SylvesVector3* v1,
    SylvesVector3* v2,
    SylvesVector3* v3,
    SylvesVector3* v4,
    SylvesVector3* v5,
    SylvesVector3* v6,
    SylvesVector3* v7,
    SylvesVector3* v8,
    SylvesError* error_out)
{
    /* First get base corners */
    SylvesVector3 base_v1, base_v2, base_v3, base_v4;
    if (!get_corners_2d(mesh, submesh, face, invert_winding,
                       &base_v1, &base_v2, &base_v3, &base_v4, error_out)) {
        return false;
    }
    
    /* TODO: Implement normal-based offset calculation */
    (void)mesh_offset1; (void)mesh_offset2;
    (void)v1; (void)v2; (void)v3; (void)v4;
    (void)v5; (void)v6; (void)v7; (void)v8;
    
    if (error_out) *error_out = SYLVES_ERROR_NOT_IMPLEMENTED;
    return false;
}

/**
 * @brief Create quad interpolation from mesh face
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    SylvesError* error_out)
{
    SylvesQuadInterpolation* interp = calloc(1, sizeof(SylvesQuadInterpolation));
    if (!interp) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    if (!get_corners_2d(mesh, submesh, face, invert_winding,
                       &interp->v1, &interp->v2, &interp->v3, &interp->v4,
                       error_out)) {
        free(interp);
        return NULL;
    }
    
    interp->is_3d = false;
    return interp;
}

/**
 * @brief Create quad prism interpolation from mesh face with offsets
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_prism_from_mesh(
    const SylvesMeshData* mesh,
    int submesh,
    int face,
    bool invert_winding,
    float mesh_offset1,
    float mesh_offset2,
    SylvesError* error_out)
{
    SylvesQuadInterpolation* interp = calloc(1, sizeof(SylvesQuadInterpolation));
    if (!interp) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    if (!get_corners_3d(mesh, submesh, face, invert_winding,
                       mesh_offset1, mesh_offset2,
                       &interp->v1, &interp->v2, &interp->v3, &interp->v4,
                       &interp->v5, &interp->v6, &interp->v7, &interp->v8,
                       error_out)) {
        free(interp);
        return NULL;
    }
    
    interp->is_3d = true;
    return interp;
}

/**
 * @brief Create quad interpolation from 4 vertices
 */
SylvesQuadInterpolation* sylves_quad_interpolation_create_2d(
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    const SylvesVector3* v3,
    const SylvesVector3* v4,
    SylvesError* error_out)
{
    if (!v1 || !v2 || !v3 || !v4) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    SylvesQuadInterpolation* interp = calloc(1, sizeof(SylvesQuadInterpolation));
    if (!interp) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    interp->v1 = *v1;
    interp->v2 = *v2;
    interp->v3 = *v3;
    interp->v4 = *v4;
    interp->is_3d = false;
    
    if (error_out) *error_out = SYLVES_SUCCESS;
    return interp;
}

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
    SylvesError* error_out)
{
    if (!v1 || !v2 || !v3 || !v4 || !v5 || !v6 || !v7 || !v8) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    SylvesQuadInterpolation* interp = calloc(1, sizeof(SylvesQuadInterpolation));
    if (!interp) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    interp->v1 = *v1;
    interp->v2 = *v2;
    interp->v3 = *v3;
    interp->v4 = *v4;
    interp->v5 = *v5;
    interp->v6 = *v6;
    interp->v7 = *v7;
    interp->v8 = *v8;
    interp->is_3d = true;
    
    if (error_out) *error_out = SYLVES_SUCCESS;
    return interp;
}

/**
 * @brief Destroy quad interpolation
 */
void sylves_quad_interpolation_destroy(SylvesQuadInterpolation* interp)
{
    free(interp);
}

/**
 * @brief Interpolate position using bilinear or trilinear interpolation
 * 
 * For 2D: Bilinear interpolation on quad, z value of p is unused
 * For 3D: Trilinear interpolation on prism
 * 
 * Following Sylves conventions:
 * - 2D: v1=(-0.5,0,-0.5), v2=(-0.5,0,0.5), v3=(0.5,0,0.5), v4=(0.5,0,-0.5)
 * - 3D: v1-v4 at y=-0.5, v5-v8 at y=0.5
 */
SylvesVector3 sylves_quad_interpolation_position(
    const SylvesQuadInterpolation* interp,
    SylvesVector3 p)
{
    if (!interp) {
        return sylves_vector3_zero();
    }
    
    if (interp->is_3d) {
        /* Trilinear interpolation */
        float x = p.x + 0.5f;
        float y = p.y + 0.5f;
        float z = p.z + 0.5f;
        
        /* Interpolate along y for each corner */
        SylvesVector3 a = lerp3(interp->v1, interp->v2, y);
        SylvesVector3 b = lerp3(interp->v4, interp->v3, y);
        SylvesVector3 c = lerp3(interp->v5, interp->v6, y);
        SylvesVector3 d = lerp3(interp->v8, interp->v7, y);
        
        /* Interpolate along x */
        SylvesVector3 e = lerp3(a, b, x);
        SylvesVector3 f = lerp3(c, d, x);
        
        /* Interpolate along z */
        return lerp3(e, f, z);
    } else {
        /* Bilinear interpolation */
        float x = p.x + 0.5f;
        float y = p.y + 0.5f;
        
        SylvesVector3 a = lerp3(interp->v1, interp->v2, y);
        SylvesVector3 b = lerp3(interp->v4, interp->v3, y);
        
        return lerp3(a, b, x);
    }
}

/**
 * @brief Calculate Jacobi matrix for the interpolation
 */
void sylves_quad_interpolation_jacobi(
    const SylvesQuadInterpolation* interp,
    SylvesVector3 p,
    SylvesMatrix4x4* jacobi)
{
    if (!jacobi || !interp) return;
    
    /* TODO: Implement proper Jacobi calculation */
    memset(jacobi, 0, sizeof(SylvesMatrix4x4));
    
    /* Set identity for now */
    jacobi->m[0] = 1.0;
    jacobi->m[5] = 1.0;
    jacobi->m[10] = 1.0;
    jacobi->m[15] = 1.0;
    
    (void)p;
}
