/**
 * @file deformation.c
 * @brief Deformation interface implementation
 */

#include "sylves/deformation.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "sylves/mesh.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief Default epsilon for numerical differentiation
 */
#define DEFORM_EPSILON 1e-3f

/**
 * @brief Internal structure for deformation context
 */
/* struct SylvesDeformation is defined in the public header */

/* Forward declaration */
void sylves_deformation_update_winding(SylvesDeformation* deform);

/* Helper functions */
static SylvesMatrix4x4 matrix_inverse_transpose(const SylvesMatrix4x4* m) {
    /* First compute inverse */
    SylvesMatrix4x4 inv;
    if (!sylves_matrix4x4_invert(m, &inv)) {
        /* Return identity if inversion fails */
        return sylves_matrix4x4_identity();
    }
    /* Then transpose */
    return sylves_matrix4x4_transpose(&inv);
}

static float matrix_determinant(const SylvesMatrix4x4* m) {
    /* 4x4 determinant calculation - matrix is in column-major order */
    /* Access as m[col * 4 + row] */
    float a00 = m->m[0], a01 = m->m[4], a02 = m->m[8], a03 = m->m[12];
    float a10 = m->m[1], a11 = m->m[5], a12 = m->m[9], a13 = m->m[13];
    float a20 = m->m[2], a21 = m->m[6], a22 = m->m[10], a23 = m->m[14];
    float a30 = m->m[3], a31 = m->m[7], a32 = m->m[11], a33 = m->m[15];
    
    float b00 = a11 * (a22 * a33 - a23 * a32) - a12 * (a21 * a33 - a23 * a31) + a13 * (a21 * a32 - a22 * a31);
    float b01 = a10 * (a22 * a33 - a23 * a32) - a12 * (a20 * a33 - a23 * a30) + a13 * (a20 * a32 - a22 * a30);
    float b02 = a10 * (a21 * a33 - a23 * a31) - a11 * (a20 * a33 - a23 * a30) + a13 * (a20 * a31 - a21 * a30);
    float b03 = a10 * (a21 * a32 - a22 * a31) - a11 * (a20 * a32 - a22 * a30) + a12 * (a20 * a31 - a21 * a30);
    
    return a00 * b00 - a01 * b01 + a02 * b02 - a03 * b03;
}

/* Default normal deformation using jacobi */
static SylvesVector3 default_deform_normal(SylvesVector3 p, SylvesVector3 v, void* context) {
    SylvesDeformation* deform = (SylvesDeformation*)context;
    SylvesMatrix4x4 jacobi;
    deform->get_jacobi(p, &jacobi, deform->context);
    
    /* Normal transform: (J^-1)^T * v */
    SylvesMatrix4x4 jacobi_it = matrix_inverse_transpose(&jacobi);
    SylvesVector3 result = sylves_matrix4x4_multiply_vector(&jacobi_it, v);
    return sylves_vector3_normalize(result);
}

/* Helper to multiply matrix by vector4 */
static SylvesVector4 matrix_multiply_vector4(const SylvesMatrix4x4* m, SylvesVector4 v) {
    /* Transform as 3D vector, preserve w */
    SylvesVector3 v3 = (SylvesVector3){v.x, v.y, v.z};
    v3 = sylves_matrix4x4_multiply_vector(m, v3);
    return (SylvesVector4){v3.x, v3.y, v3.z, v.w};
}

/* Default tangent deformation using jacobi */
static SylvesVector4 default_deform_tangent(SylvesVector3 p, SylvesVector4 v, void* context) {
    SylvesDeformation* deform = (SylvesDeformation*)context;
    SylvesMatrix4x4 jacobi;
    deform->get_jacobi(p, &jacobi, deform->context);
    
    /* Tangent transform: J * v */
    return matrix_multiply_vector4(&jacobi, v);
}

/* Numerical differentiation jacobi */
static void numerical_jacobi(SylvesVector3 p, SylvesMatrix4x4* jacobi, void* context) {
    SylvesDeformation* deform = (SylvesDeformation*)context;
    float h = deform->epsilon;
    
    /* Get center point */
    SylvesVector3 center = deform->deform_point(p, deform->context);
    
    /* Compute partial derivatives */
    SylvesVector3 px = (SylvesVector3){p.x + h, p.y, p.z};
    SylvesVector3 py = (SylvesVector3){p.x, p.y + h, p.z};
    SylvesVector3 pz = (SylvesVector3){p.x, p.y, p.z + h};
    
    SylvesVector3 dx = sylves_vector3_subtract(deform->deform_point(px, deform->context), center);
    SylvesVector3 dy = sylves_vector3_subtract(deform->deform_point(py, deform->context), center);
    SylvesVector3 dz = sylves_vector3_subtract(deform->deform_point(pz, deform->context), center);
    
    dx = sylves_vector3_scale(dx, 1.0f / h);
    dy = sylves_vector3_scale(dy, 1.0f / h);
    dz = sylves_vector3_scale(dz, 1.0f / h);
    
    /* Build jacobi matrix - column major order */
    jacobi->m[0] = dx.x; jacobi->m[4] = dy.x; jacobi->m[8] = dz.x; jacobi->m[12] = center.x;
    jacobi->m[1] = dx.y; jacobi->m[5] = dy.y; jacobi->m[9] = dz.y; jacobi->m[13] = center.y;
    jacobi->m[2] = dx.z; jacobi->m[6] = dy.z; jacobi->m[10] = dz.z; jacobi->m[14] = center.z;
    jacobi->m[3] = 0;    jacobi->m[7] = 0;    jacobi->m[11] = 0;    jacobi->m[15] = 1;
}

/* Identity deformation functions */
static SylvesVector3 identity_deform_point(SylvesVector3 p, void* context) {
    (void)context;
    return p;
}

static SylvesVector3 identity_deform_normal(SylvesVector3 p, SylvesVector3 v, void* context) {
    (void)p;
    (void)context;
    return v;
}

static SylvesVector4 identity_deform_tangent(SylvesVector3 p, SylvesVector4 v, void* context) {
    (void)p;
    (void)context;
    return v;
}

static void identity_get_jacobi(SylvesVector3 p, SylvesMatrix4x4* jacobi, void* context) {
    (void)context;
    *jacobi = sylves_matrix4x4_translation(p);
}

/* Create identity deformation singleton */
static SylvesDeformation g_identity_deformation;

/* Initialize identity deformation */
static void init_identity_deformation(void) {
    static bool initialized = false;
    if (initialized) return;
    
    g_identity_deformation.deform_point = identity_deform_point;
    g_identity_deformation.deform_normal = identity_deform_normal;
    g_identity_deformation.deform_tangent = identity_deform_tangent;
    g_identity_deformation.get_jacobi = identity_get_jacobi;
    g_identity_deformation.context = NULL;
    g_identity_deformation.pre_deform = sylves_matrix4x4_identity();
    g_identity_deformation.post_deform = sylves_matrix4x4_identity();
    g_identity_deformation.pre_deform_it = sylves_matrix4x4_identity();
    g_identity_deformation.post_deform_it = sylves_matrix4x4_identity();
    g_identity_deformation.invert_winding = false;
    g_identity_deformation.inner_invert_winding = false;
    g_identity_deformation.epsilon = DEFORM_EPSILON;
    
    initialized = true;
}

SylvesDeformation* sylves_deformation_identity(void) {
    init_identity_deformation();
    return &g_identity_deformation;
}

SylvesDeformation* sylves_deformation_create(
    SylvesDeformPointFunc deform_point,
    SylvesDeformNormalFunc deform_normal,
    SylvesDeformTangentFunc deform_tangent,
    SylvesGetJacobiFunc get_jacobi,
    void* context,
    bool invert_winding,
    SylvesError* error_out
) {
    if (!deform_point) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    SylvesDeformation* deform = calloc(1, sizeof(SylvesDeformation));
    if (!deform) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    deform->deform_point = deform_point;
    deform->context = context;
    deform->inner_invert_winding = invert_winding;
    deform->epsilon = DEFORM_EPSILON;
    
    /* Set up matrices */
    deform->pre_deform = sylves_matrix4x4_identity();
    deform->post_deform = sylves_matrix4x4_identity();
    deform->pre_deform_it = sylves_matrix4x4_identity();
    deform->post_deform_it = sylves_matrix4x4_identity();
    
    /* Set up functions */
    if (get_jacobi) {
        deform->get_jacobi = get_jacobi;
        
        /* Use provided or default normal/tangent functions */
        if (deform_normal) {
            deform->deform_normal = deform_normal;
        } else {
            deform->deform_normal = default_deform_normal;
        }
        
        if (deform_tangent) {
            deform->deform_tangent = deform_tangent;
        } else {
            deform->deform_tangent = default_deform_tangent;
        }
    } else {
        /* Use numerical differentiation */
        deform->get_jacobi = numerical_jacobi;
        deform->deform_normal = default_deform_normal;
        deform->deform_tangent = default_deform_tangent;
    }
    
    /* Update winding */
    sylves_deformation_update_winding(deform);
    
    return deform;
}

SylvesDeformation* sylves_deformation_create_with_jacobi(
    SylvesDeformPointFunc deform_point,
    SylvesGetJacobiFunc get_jacobi,
    bool invert_winding,
    void* context,
    SylvesError* error_out
) {
    return sylves_deformation_create(
        deform_point,
        NULL,  /* Use default normal */
        NULL,  /* Use default tangent */
        get_jacobi,
        context,
        invert_winding,
        error_out
    );
}

SylvesDeformation* sylves_deformation_create_numerical(
    SylvesDeformPointFunc deform_point,
    float step,
    bool invert_winding,
    void* context,
    SylvesError* error_out
) {
    SylvesDeformation* deform = sylves_deformation_create(
        deform_point,
        NULL,  /* Use default normal */
        NULL,  /* Use default tangent */
        NULL,  /* Use numerical jacobi */
        context,
        invert_winding,
        error_out
    );
    
    if (deform && step > 0) {
        deform->epsilon = step;
    }
    
    return deform;
}

void sylves_deformation_destroy(SylvesDeformation* deform) {
    if (deform && deform != &g_identity_deformation) {
        free(deform);
    }
}

SylvesDeformation* sylves_deformation_clone(const SylvesDeformation* deform, SylvesError* error_out) {
    if (!deform) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }
    
    if (deform == &g_identity_deformation) {
        return &g_identity_deformation;
    }
    
    SylvesDeformation* clone = malloc(sizeof(SylvesDeformation));
    if (!clone) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    memcpy(clone, deform, sizeof(SylvesDeformation));
    return clone;
}

void sylves_deformation_set_pre_transform(SylvesDeformation* deform, const SylvesMatrix4x4* transform) {
    if (!deform || deform == &g_identity_deformation) return;
    
    deform->pre_deform = *transform;
    deform->pre_deform_it = matrix_inverse_transpose(transform);
    sylves_deformation_update_winding(deform);
}

void sylves_deformation_set_post_transform(SylvesDeformation* deform, const SylvesMatrix4x4* transform) {
    if (!deform || deform == &g_identity_deformation) return;
    
    deform->post_deform = *transform;
    deform->post_deform_it = matrix_inverse_transpose(transform);
    sylves_deformation_update_winding(deform);
}

void sylves_deformation_get_pre_transform(const SylvesDeformation* deform, SylvesMatrix4x4* transform) {
    if (!deform || !transform) return;
    *transform = deform->pre_deform;
}

void sylves_deformation_get_post_transform(const SylvesDeformation* deform, SylvesMatrix4x4* transform) {
    if (!deform || !transform) return;
    *transform = deform->post_deform;
}

void sylves_deformation_update_winding(SylvesDeformation* deform) {
    if (!deform || deform == &g_identity_deformation) return;
    
    bool pre_negative = matrix_determinant(&deform->pre_deform) < 0;
    bool post_negative = matrix_determinant(&deform->post_deform) < 0;
    
    deform->invert_winding = deform->inner_invert_winding ^ pre_negative ^ post_negative;
}

bool sylves_deformation_get_invert_winding(const SylvesDeformation* deform) {
    return deform ? deform->invert_winding : false;
}

SylvesVector3 sylves_deformation_deform_point(const SylvesDeformation* deform, SylvesVector3 p) {
    if (!deform) return p;
    
    /* Apply pre-transform */
    p = sylves_matrix4x4_multiply_point(&deform->pre_deform, p);
    
    /* Apply inner deformation */
    p = deform->deform_point(p, deform->context);
    
    /* Apply post-transform */
    p = sylves_matrix4x4_multiply_point(&deform->post_deform, p);
    
    return p;
}

SylvesVector3 sylves_deformation_deform_normal(const SylvesDeformation* deform, SylvesVector3 p, SylvesVector3 v) {
    if (!deform) return v;
    
    /* Transform point to inner space */
    SylvesVector3 inner_p = sylves_matrix4x4_multiply_point(&deform->pre_deform, p);
    
    /* Transform normal with inverse transpose */
    v = sylves_matrix4x4_multiply_vector(&deform->pre_deform_it, v);
    
    /* Apply inner deformation */
    v = deform->deform_normal(inner_p, v, deform);
    
    /* Transform with post inverse transpose */
    v = sylves_matrix4x4_multiply_vector(&deform->post_deform_it, v);
    
    return v;
}

SylvesVector4 sylves_deformation_deform_tangent(const SylvesDeformation* deform, SylvesVector3 p, SylvesVector4 t) {
    if (!deform) return t;
    
    /* Transform point to inner space */
    SylvesVector3 inner_p = sylves_matrix4x4_multiply_point(&deform->pre_deform, p);
    
    /* Transform tangent direction */
    SylvesVector3 t3 = (SylvesVector3){t.x, t.y, t.z};
    t3 = sylves_matrix4x4_multiply_vector(&deform->pre_deform, t3);
    SylvesVector4 t4 = (SylvesVector4){t3.x, t3.y, t3.z, t.w};
    
    /* Apply inner deformation */
    t4 = deform->deform_tangent(inner_p, t4, deform);
    
    /* Transform with post matrix */
    t3 = (SylvesVector3){t4.x, t4.y, t4.z};
    t3 = sylves_matrix4x4_multiply_vector(&deform->post_deform, t3);
    
    return (SylvesVector4){t3.x, t3.y, t3.z, t4.w};
}

void sylves_deformation_get_jacobi(const SylvesDeformation* deform, SylvesVector3 p, SylvesMatrix4x4* jacobi) {
    if (!deform || !jacobi) return;
    
    /* Transform point to inner space */
    SylvesVector3 inner_p = sylves_matrix4x4_multiply_point(&deform->pre_deform, p);
    
    /* Get inner jacobi */
    SylvesMatrix4x4 inner_jacobi;
    deform->get_jacobi(inner_p, &inner_jacobi, deform);
    
    /* Chain rule: J = post * inner_J * pre */
    SylvesMatrix4x4 temp = sylves_matrix4x4_multiply(&inner_jacobi, &deform->pre_deform);
    *jacobi = sylves_matrix4x4_multiply(&deform->post_deform, &temp);
}

SylvesDeformation* sylves_deformation_pre_multiply(const SylvesDeformation* deform, const SylvesMatrix4x4* transform, SylvesError* error_out) {
    SylvesDeformation* result = sylves_deformation_clone(deform, error_out);
    if (!result) return NULL;
    
    SylvesMatrix4x4 new_pre = sylves_matrix4x4_multiply(&result->pre_deform, transform);
    sylves_deformation_set_pre_transform(result, &new_pre);
    
    return result;
}

SylvesDeformation* sylves_deformation_post_multiply(const SylvesMatrix4x4* transform, const SylvesDeformation* deform, SylvesError* error_out) {
    SylvesDeformation* result = sylves_deformation_clone(deform, error_out);
    if (!result) return NULL;
    
    SylvesMatrix4x4 new_post = sylves_matrix4x4_multiply(transform, &result->post_deform);
    sylves_deformation_set_post_transform(result, &new_post);
    
    return result;
}

void sylves_deformation_deform_mesh(
    const SylvesDeformation* deform,
    const SylvesMeshData* mesh,
    SylvesMeshData* out_mesh,
    SylvesError* error_out
) {
    if (!deform || !mesh || !out_mesh) {
        if (error_out) {
            *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    /* Copy mesh structure */
    *out_mesh = *mesh;
    
    /* Allocate new vertex data */
    int vertex_count = (int)mesh->vertex_count;
    out_mesh->vertices = malloc((size_t)vertex_count * sizeof(SylvesVector3));
    if (!out_mesh->vertices) {
        if (error_out) {
            *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        }
        return;
    }
    
    /* Transform vertices */
    for (int i = 0; i < vertex_count; i++) {
        out_mesh->vertices[i] = sylves_deformation_deform_point(deform, mesh->vertices[i]);
    }
    
    /* Transform normals if present */
    if (mesh->normals) {
        out_mesh->normals = malloc((size_t)vertex_count * sizeof(SylvesVector3));
        if (!out_mesh->normals) {
            free(out_mesh->vertices);
            if (error_out) {
                *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
            }
            return;
        }
        
        for (int i = 0; i < vertex_count; i++) {
            out_mesh->normals[i] = sylves_deformation_deform_normal(deform, mesh->vertices[i], mesh->normals[i]);
        }
    }
    
    /* Faces and UVs are left as-is (topology unchanged). */
    out_mesh->faces = mesh->faces;
    out_mesh->face_count = mesh->face_count;
    out_mesh->uvs = mesh->uvs;
}
