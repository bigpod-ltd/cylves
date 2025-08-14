/**
 * @file triangle_interpolation.c
 * @brief Implementation of triangle interpolation methods
 */

#include "sylves/triangle_interpolation.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "sylves/errors.h"
#include "sylves/mesh_data.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

static const float SQRT3 = 1.73205080756888f;

static float barycentric_det2x2(float ax, float bx, float ay, float by) {
    return ax * by - ay * bx;
}

static SylvesVector3 std_barycentric(SylvesVector2 p) {
    const float o = 1.0f / 3.0f;
    const float a = 1.0f / 3.0f * SQRT3;
    const float b = 2.0f / 3.0f * SQRT3;
    const float c = 0.57735026919f * SQRT3;
    
    return (SylvesVector3) {
        o + c * p.x - a * p.y,
        o +         b * p.y,
        o - c * p.x - a * p.y
    };
}

static void std_barycentric_diff(SylvesVector3* dbdx, SylvesVector3* dbdy) {
    const float a = 1.0f / 3.0f * SQRT3;
    const float c = 0.57735026919f * SQRT3;
    
    *dbdx = (SylvesVector3) {c, 0, -c};
    *dbdy = (SylvesVector3) {-a, 2.0f / 3.0f * SQRT3, -a};
}

static SylvesVector3 barycentric_coord(SylvesVector2 p, SylvesVector2 a, SylvesVector2 b, SylvesVector2 c) {
    SylvesVector2 v0 = {b.x - a.x, b.y - a.y};
    SylvesVector2 v1 = {c.x - a.x, c.y - a.y};
    SylvesVector2 v2 = {p.x - a.x, p.y - a.y};
    
    float d00 = v0.x * v0.x + v0.y * v0.y;
    float d01 = v0.x * v1.x + v0.y * v1.y;
    float d11 = v1.x * v1.x + v1.y * v1.y;
    float d20 = v2.x * v0.x + v2.y * v0.y;
    float d21 = v2.x * v1.x + v2.y * v1.y;
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return (SylvesVector3){u, v, w};
}

SylvesVector3 sylves_triangle_interpolation_position(const SylvesTriangleInterpolation* interp, SylvesVector3 p) {
    if (!interp) return p;
    
    if (interp->is_3d) {
        /* 3D prism interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        float z1 = 0.5f - p.z;
        float z2 = 0.5f + p.z;
        
        SylvesVector3 u1 = {
            bary.x * interp->v1.x + bary.y * interp->v2.x + bary.z * interp->v3.x,
            bary.x * interp->v1.y + bary.y * interp->v2.y + bary.z * interp->v3.y,
            bary.x * interp->v1.z + bary.y * interp->v2.z + bary.z * interp->v3.z
        };
        
        SylvesVector3 u2 = {
            bary.x * interp->v4.x + bary.y * interp->v5.x + bary.z * interp->v6.x,
            bary.x * interp->v4.y + bary.y * interp->v5.y + bary.z * interp->v6.y,
            bary.x * interp->v4.z + bary.y * interp->v5.z + bary.z * interp->v6.z
        };
        
        return (SylvesVector3){
            z1 * u1.x + z2 * u2.x,
            z1 * u1.y + z2 * u2.y,
            z1 * u1.z + z2 * u2.z
        };
    } else {
        /* 2D triangle interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        
        return (SylvesVector3){
            interp->v1.x * bary.x + interp->v2.x * bary.y + interp->v3.x * bary.z,
            interp->v1.y * bary.x + interp->v2.y * bary.y + interp->v3.y * bary.z,
            interp->v1.z * bary.x + interp->v2.z * bary.y + interp->v3.z * bary.z
        };
    }
}

void sylves_triangle_interpolation_jacobi(const SylvesTriangleInterpolation* interp, SylvesVector3 p, SylvesMatrix4x4* jacobi) {
    if (!interp || !jacobi) return;
    
    SylvesVector3 dbdx, dbdy;
    std_barycentric_diff(&dbdx, &dbdy);
    
    if (interp->is_3d) {
        /* 3D prism jacobi */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        float z1 = 0.5f - p.z;
        float z2 = 0.5f + p.z;
        
        /* Compute derivatives with respect to barycentric coordinates */
        SylvesVector3 u1 = {
            bary.x * interp->v1.x + bary.y * interp->v2.x + bary.z * interp->v3.x,
            bary.x * interp->v1.y + bary.y * interp->v2.y + bary.z * interp->v3.y,
            bary.x * interp->v1.z + bary.y * interp->v2.z + bary.z * interp->v3.z
        };
        
        SylvesVector3 u2 = {
            bary.x * interp->v4.x + bary.y * interp->v5.x + bary.z * interp->v6.x,
            bary.x * interp->v4.y + bary.y * interp->v5.y + bary.z * interp->v6.y,
            bary.x * interp->v4.z + bary.y * interp->v5.z + bary.z * interp->v6.z
        };
        
        SylvesVector3 du1dx = {
            dbdx.x * interp->v1.x + dbdx.y * interp->v2.x + dbdx.z * interp->v3.x,
            dbdx.x * interp->v1.y + dbdx.y * interp->v2.y + dbdx.z * interp->v3.y,
            dbdx.x * interp->v1.z + dbdx.y * interp->v2.z + dbdx.z * interp->v3.z
        };
        
        SylvesVector3 du2dx = {
            dbdx.x * interp->v4.x + dbdx.y * interp->v5.x + dbdx.z * interp->v6.x,
            dbdx.x * interp->v4.y + dbdx.y * interp->v5.y + dbdx.z * interp->v6.y,
            dbdx.x * interp->v4.z + dbdx.y * interp->v5.z + dbdx.z * interp->v6.z
        };
        
        SylvesVector3 du1dy = {
            dbdy.x * interp->v1.x + dbdy.y * interp->v2.x + dbdy.z * interp->v3.x,
            dbdy.x * interp->v1.y + dbdy.y * interp->v2.y + dbdy.z * interp->v3.y,
            dbdy.x * interp->v1.z + dbdy.y * interp->v2.z + dbdy.z * interp->v3.z
        };
        
        SylvesVector3 du2dy = {
            dbdy.x * interp->v4.x + dbdy.y * interp->v5.x + dbdy.z * interp->v6.x,
            dbdy.x * interp->v4.y + dbdy.y * interp->v5.y + dbdy.z * interp->v6.y,
            dbdy.x * interp->v4.z + dbdy.y * interp->v5.z + dbdy.z * interp->v6.z
        };
        
        SylvesVector3 o = {
            z1 * u1.x + z2 * u2.x,
            z1 * u1.y + z2 * u2.y,
            z1 * u1.z + z2 * u2.z
        };
        
        SylvesVector3 dodx = {
            z1 * du1dx.x + z2 * du2dx.x,
            z1 * du1dx.y + z2 * du2dx.y,
            z1 * du1dx.z + z2 * du2dx.z
        };
        
        SylvesVector3 dody = {
            z1 * du1dy.x + z2 * du2dy.x,
            z1 * du1dy.y + z2 * du2dy.y,
            z1 * du1dy.z + z2 * du2dy.z
        };
        
        SylvesVector3 dodz = {
            u2.x - u1.x,
            u2.y - u1.y,
            u2.z - u1.z
        };
        
        /* Build jacobi matrix - column major order */
        jacobi->m[0] = dodx.x; jacobi->m[4] = dody.x; jacobi->m[8] = dodz.x; jacobi->m[12] = o.x;
        jacobi->m[1] = dodx.y; jacobi->m[5] = dody.y; jacobi->m[9] = dodz.y; jacobi->m[13] = o.y;
        jacobi->m[2] = dodx.z; jacobi->m[6] = dody.z; jacobi->m[10] = dodz.z; jacobi->m[14] = o.z;
        jacobi->m[3] = 0;      jacobi->m[7] = 0;      jacobi->m[11] = 0;      jacobi->m[15] = 1;
    } else {
        /* 2D triangle jacobi */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        
        SylvesVector3 o = {
            bary.x * interp->v1.x + bary.y * interp->v2.x + bary.z * interp->v3.x,
            bary.x * interp->v1.y + bary.y * interp->v2.y + bary.z * interp->v3.y,
            bary.x * interp->v1.z + bary.y * interp->v2.z + bary.z * interp->v3.z
        };
        
        SylvesVector3 dodx = {
            dbdx.x * interp->v1.x + dbdx.y * interp->v2.x + dbdx.z * interp->v3.x,
            dbdx.x * interp->v1.y + dbdx.y * interp->v2.y + dbdx.z * interp->v3.y,
            dbdx.x * interp->v1.z + dbdx.y * interp->v2.z + dbdx.z * interp->v3.z
        };
        
        SylvesVector3 dody = {
            dbdy.x * interp->v1.x + dbdy.y * interp->v2.x + dbdy.z * interp->v3.x,
            dbdy.x * interp->v1.y + dbdy.y * interp->v2.y + dbdy.z * interp->v3.y,
            dbdy.x * interp->v1.z + dbdy.y * interp->v2.z + dbdy.z * interp->v3.z
        };
        
        /* Build jacobi matrix - column major order */
        jacobi->m[0] = dodx.x; jacobi->m[4] = dody.x; jacobi->m[8] = 0;  jacobi->m[12] = o.x;
        jacobi->m[1] = dodx.y; jacobi->m[5] = dody.y; jacobi->m[9] = 0;  jacobi->m[13] = o.y;
        jacobi->m[2] = dodx.z; jacobi->m[6] = dody.z; jacobi->m[10] = 0; jacobi->m[14] = o.z;
        jacobi->m[3] = 0;      jacobi->m[7] = 0;      jacobi->m[11] = 0; jacobi->m[15] = 1;
    }
}

SylvesTriangleInterpolation* sylves_triangle_interpolation_create_from_mesh(const SylvesMeshData* mesh, int submesh __attribute__((unused)), int face, bool invert_winding, SylvesError* error_out) {
    if (!mesh || face >= mesh->face_count) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    const SylvesMeshFace* mesh_face = &mesh->faces[face];
    if (mesh_face->vertex_count != 3) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    /* Get vertex indices */
    int i1, i2, i3;
    if (invert_winding) {
        i1 = mesh_face->vertices[2];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[0];
    } else {
        i1 = mesh_face->vertices[0];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[2];
    }
    
    /* Get vertices */
    SylvesVector3 v1 = mesh->vertices[i1];
    SylvesVector3 v2 = mesh->vertices[i2];
    SylvesVector3 v3 = mesh->vertices[i3];
    
    return sylves_triangle_interpolation_create_2d(&v1, &v2, &v3, error_out);
}

SylvesTriangleInterpolation* sylves_triangle_interpolation_create_prism_from_mesh(const SylvesMeshData* mesh, int submesh __attribute__((unused)), int face, bool invert_winding, float mesh_offset1, float mesh_offset2, SylvesError* error_out) {
    if (!mesh || face >= mesh->face_count) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    const SylvesMeshFace* mesh_face = &mesh->faces[face];
    if (mesh_face->vertex_count != 3) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    if (!mesh->normals) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    /* Get vertex indices */
    int i1, i2, i3;
    if (invert_winding) {
        i1 = mesh_face->vertices[2];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[0];
    } else {
        i1 = mesh_face->vertices[0];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[2];
    }
    
    /* Get vertices with offsets */
    SylvesVector3 v1 = sylves_vector3_add(mesh->vertices[i1], sylves_vector3_scale(mesh->normals[i1], mesh_offset1));
    SylvesVector3 v2 = sylves_vector3_add(mesh->vertices[i2], sylves_vector3_scale(mesh->normals[i2], mesh_offset1));
    SylvesVector3 v3 = sylves_vector3_add(mesh->vertices[i3], sylves_vector3_scale(mesh->normals[i3], mesh_offset1));
    SylvesVector3 v4 = sylves_vector3_add(mesh->vertices[i1], sylves_vector3_scale(mesh->normals[i1], mesh_offset2));
    SylvesVector3 v5 = sylves_vector3_add(mesh->vertices[i2], sylves_vector3_scale(mesh->normals[i2], mesh_offset2));
    SylvesVector3 v6 = sylves_vector3_add(mesh->vertices[i3], sylves_vector3_scale(mesh->normals[i3], mesh_offset2));
    
    return sylves_triangle_interpolation_create_3d(&v1, &v2, &v3, &v4, &v5, &v6, error_out);
}

SylvesTriangleInterpolation* sylves_triangle_interpolation_create_2d(const SylvesVector3* v1, const SylvesVector3* v2, const SylvesVector3* v3, SylvesError* error_out) {
    if (!v1 || !v2 || !v3) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    SylvesTriangleInterpolation* interp = calloc(1, sizeof(SylvesTriangleInterpolation));
    if (!interp) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    interp->v1 = *v1;
    interp->v2 = *v2;
    interp->v3 = *v3;
    interp->is_3d = false;
    
    return interp;
}

SylvesTriangleInterpolation* sylves_triangle_interpolation_create_3d(const SylvesVector3* v1, const SylvesVector3* v2, const SylvesVector3* v3, const SylvesVector3* v4, const SylvesVector3* v5, const SylvesVector3* v6, SylvesError* error_out) {
    if (!v1 || !v2 || !v3 || !v4 || !v5 || !v6) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    SylvesTriangleInterpolation* interp = calloc(1, sizeof(SylvesTriangleInterpolation));
    if (!interp) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    interp->v1 = *v1;
    interp->v2 = *v2;
    interp->v3 = *v3;
    interp->v4 = *v4;
    interp->v5 = *v5;
    interp->v6 = *v6;
    interp->is_3d = true;
    
    return interp;
}

void sylves_triangle_interpolation_destroy(SylvesTriangleInterpolation* interp) {
    if (interp) {
        free(interp);
    }
}

SylvesVector2 sylves_triangle_interp_vector2(const SylvesTriangleAttributeInterp* interp, SylvesVector3 p) {
    if (!interp || interp->dimensions != 2) return (SylvesVector2){0, 0};
    
    if (interp->is_3d) {
        /* 3D prism interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        float z1 = 0.5f - p.z;
        float z2 = 0.5f + p.z;
        
        SylvesVector2 u1 = {
            bary.x * interp->values.v2[0].x + bary.y * interp->values.v2[1].x + bary.z * interp->values.v2[2].x,
            bary.x * interp->values.v2[0].y + bary.y * interp->values.v2[1].y + bary.z * interp->values.v2[2].y
        };
        
        SylvesVector2 u2 = {
            bary.x * interp->values.v2[3].x + bary.y * interp->values.v2[4].x + bary.z * interp->values.v2[5].x,
            bary.x * interp->values.v2[3].y + bary.y * interp->values.v2[4].y + bary.z * interp->values.v2[5].y
        };
        
        return (SylvesVector2){
            z1 * u1.x + z2 * u2.x,
            z1 * u1.y + z2 * u2.y
        };
    } else {
        /* 2D triangle interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        
        return (SylvesVector2){
            interp->values.v2[0].x * bary.x + interp->values.v2[1].x * bary.y + interp->values.v2[2].x * bary.z,
            interp->values.v2[0].y * bary.x + interp->values.v2[1].y * bary.y + interp->values.v2[2].y * bary.z
        };
    }
}

SylvesVector3 sylves_triangle_interp_vector3(const SylvesTriangleAttributeInterp* interp, SylvesVector3 p) {
    if (!interp || interp->dimensions != 3) return (SylvesVector3){0, 0, 0};
    
    if (interp->is_3d) {
        /* 3D prism interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        float z1 = 0.5f - p.z;
        float z2 = 0.5f + p.z;
        
        SylvesVector3 u1 = {
            bary.x * interp->values.v3[0].x + bary.y * interp->values.v3[1].x + bary.z * interp->values.v3[2].x,
            bary.x * interp->values.v3[0].y + bary.y * interp->values.v3[1].y + bary.z * interp->values.v3[2].y,
            bary.x * interp->values.v3[0].z + bary.y * interp->values.v3[1].z + bary.z * interp->values.v3[2].z
        };
        
        SylvesVector3 u2 = {
            bary.x * interp->values.v3[3].x + bary.y * interp->values.v3[4].x + bary.z * interp->values.v3[5].x,
            bary.x * interp->values.v3[3].y + bary.y * interp->values.v3[4].y + bary.z * interp->values.v3[5].y,
            bary.x * interp->values.v3[3].z + bary.y * interp->values.v3[4].z + bary.z * interp->values.v3[5].z
        };
        
        return (SylvesVector3){
            z1 * u1.x + z2 * u2.x,
            z1 * u1.y + z2 * u2.y,
            z1 * u1.z + z2 * u2.z
        };
    } else {
        /* 2D triangle interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        
        return (SylvesVector3){
            interp->values.v3[0].x * bary.x + interp->values.v3[1].x * bary.y + interp->values.v3[2].x * bary.z,
            interp->values.v3[0].y * bary.x + interp->values.v3[1].y * bary.y + interp->values.v3[2].y * bary.z,
            interp->values.v3[0].z * bary.x + interp->values.v3[1].z * bary.y + interp->values.v3[2].z * bary.z
        };
    }
}

SylvesVector4 sylves_triangle_interp_vector4(const SylvesTriangleAttributeInterp* interp, SylvesVector3 p) {
    if (!interp || interp->dimensions != 4) return (SylvesVector4){0, 0, 0, 0};
    
    if (interp->is_3d) {
        /* 3D prism interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        float z1 = 0.5f - p.z;
        float z2 = 0.5f + p.z;
        
        SylvesVector4 u1 = {
            bary.x * interp->values.v4[0].x + bary.y * interp->values.v4[1].x + bary.z * interp->values.v4[2].x,
            bary.x * interp->values.v4[0].y + bary.y * interp->values.v4[1].y + bary.z * interp->values.v4[2].y,
            bary.x * interp->values.v4[0].z + bary.y * interp->values.v4[1].z + bary.z * interp->values.v4[2].z,
            bary.x * interp->values.v4[0].w + bary.y * interp->values.v4[1].w + bary.z * interp->values.v4[2].w
        };
        
        SylvesVector4 u2 = {
            bary.x * interp->values.v4[3].x + bary.y * interp->values.v4[4].x + bary.z * interp->values.v4[5].x,
            bary.x * interp->values.v4[3].y + bary.y * interp->values.v4[4].y + bary.z * interp->values.v4[5].y,
            bary.x * interp->values.v4[3].z + bary.y * interp->values.v4[4].z + bary.z * interp->values.v4[5].z,
            bary.x * interp->values.v4[3].w + bary.y * interp->values.v4[4].w + bary.z * interp->values.v4[5].w
        };
        
        return (SylvesVector4){
            z1 * u1.x + z2 * u2.x,
            z1 * u1.y + z2 * u2.y,
            z1 * u1.z + z2 * u2.z,
            z1 * u1.w + z2 * u2.w
        };
    } else {
        /* 2D triangle interpolation */
        SylvesVector2 p2D = {p.x, p.y};
        SylvesVector3 bary = std_barycentric(p2D);
        
        return (SylvesVector4){
            interp->values.v4[0].x * bary.x + interp->values.v4[1].x * bary.y + interp->values.v4[2].x * bary.z,
            interp->values.v4[0].y * bary.x + interp->values.v4[1].y * bary.y + interp->values.v4[2].y * bary.z,
            interp->values.v4[0].z * bary.x + interp->values.v4[1].z * bary.y + interp->values.v4[2].z * bary.z,
            interp->values.v4[0].w * bary.x + interp->values.v4[1].w * bary.y + interp->values.v4[2].w * bary.z
        };
    }
}

SylvesTriangleAttributeInterp* sylves_triangle_interp_normals_create(const SylvesMeshData* mesh, int submesh __attribute__((unused)), int face, bool invert_winding, SylvesError* error_out) {
    if (!mesh || !mesh->normals || face >= mesh->face_count) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    const SylvesMeshFace* mesh_face = &mesh->faces[face];
    if (mesh_face->vertex_count != 3) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    SylvesTriangleAttributeInterp* interp = calloc(1, sizeof(SylvesTriangleAttributeInterp));
    if (!interp) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    /* Get vertex indices */
    int i1, i2, i3;
    if (invert_winding) {
        i1 = mesh_face->vertices[2];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[0];
    } else {
        i1 = mesh_face->vertices[0];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[2];
    }
    
    /* Copy normal values */
    interp->values.v3[0] = mesh->normals[i1];
    interp->values.v3[1] = mesh->normals[i2];
    interp->values.v3[2] = mesh->normals[i3];
    interp->is_3d = false;
    interp->dimensions = 3;
    
    return interp;
}

SylvesTriangleAttributeInterp* sylves_triangle_interp_tangents_create(const SylvesMeshData* mesh __attribute__((unused)), int submesh __attribute__((unused)), int face __attribute__((unused)), bool invert_winding __attribute__((unused)), SylvesError* error_out) {
    /* SylvesMeshData doesn't have tangents field */
    if (error_out) {
        *error_out = SYLVES_ERROR_NOT_SUPPORTED;
    }
    return NULL;
}

SylvesTriangleAttributeInterp* sylves_triangle_interp_uvs_create(const SylvesMeshData* mesh, int submesh __attribute__((unused)), int face, bool invert_winding, SylvesError* error_out) {
    if (!mesh || !mesh->uvs || face >= mesh->face_count) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    const SylvesMeshFace* mesh_face = &mesh->faces[face];
    if (mesh_face->vertex_count != 3) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    SylvesTriangleAttributeInterp* interp = calloc(1, sizeof(SylvesTriangleAttributeInterp));
    if (!interp) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    /* Get vertex indices */
    int i1, i2, i3;
    if (invert_winding) {
        i1 = mesh_face->vertices[2];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[0];
    } else {
        i1 = mesh_face->vertices[0];
        i2 = mesh_face->vertices[1];
        i3 = mesh_face->vertices[2];
    }
    
    /* Copy UV values */
    interp->values.v2[0] = mesh->uvs[i1];
    interp->values.v2[1] = mesh->uvs[i2];
    interp->values.v2[2] = mesh->uvs[i3];
    interp->is_3d = false;
    interp->dimensions = 2;
    
    return interp;
}

void sylves_triangle_attribute_interp_destroy(SylvesTriangleAttributeInterp* interp) {
    if (interp) {
        free(interp);
    }
}
