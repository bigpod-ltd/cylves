/**
 * @file deformation_utils.c
 * @brief Deformation utility functions
 */

#include "sylves/deformation_utils.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "sylves/errors.h"
#include "sylves/deformation.h"
#include <math.h>
#include <stdlib.h>

/* Build a cylindrical deformation around Z axis at given axis point and radius.
 * This example constructs a deformation using numerical jacobian based on a point-mapper. */

typedef struct CylContext {
    SylvesVector3 axis;
    float radius;
} CylContext;

static SylvesVector3 cyl_map(SylvesVector3 p, void* ctx) {
    CylContext* c = (CylContext*)ctx;
    /* Translate relative to axis.x, axis.y in XY plane, preserve z */
    float dx = (float)(p.x - c->axis.x);
    float dy = (float)(p.y - c->axis.y);
    float theta = atan2f(dy, dx);
    SylvesVector3 out = {
        c->axis.x + c->radius * cosf(theta),
        c->axis.y + c->radius * sinf(theta),
        p.z
    };
    return out;
}

SylvesDeformation* sylves_deformation_cylindrical(const SylvesVector3* axis, float radius, SylvesError* error_out) {
    if (!axis || radius <= 0) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    CylContext* ctx = (CylContext*)malloc(sizeof(CylContext));
    if (!ctx) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ctx->axis = *axis;
    ctx->radius = radius;
    /* Construct deformation with numerical jacobian */
    SylvesDeformation* d = sylves_deformation_create(
        /* deform_point */ cyl_map,
        /* deform_normal */ NULL,
        /* deform_tangent */ NULL,
        /* get_jacobi */ NULL,
        /* context */ ctx,
        /* invert_winding */ false,
        /* error_out */ error_out
    );
    if (!d) {
        free(ctx);
        return NULL;
    }
    return d;
}

typedef struct SphContext {
    SylvesVector3 center;
    float radius;
} SphContext;

static SylvesVector3 sph_map(SylvesVector3 p, void* ctx) {
    SphContext* c = (SphContext*)ctx;
    /* Map p direction to sphere of radius around center */
    SylvesVector3 v = { p.x - c->center.x, p.y - c->center.y, p.z - c->center.z };
    double len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0) {
        SylvesVector3 out = { c->center.x + c->radius, c->center.y, c->center.z };
        return out;
    }
    double s = c->radius / len;
    SylvesVector3 out = { c->center.x + (float)(v.x * s), c->center.y + (float)(v.y * s), c->center.z + (float)(v.z * s) };
    return out;
}

SylvesDeformation* sylves_deformation_spherical(const SylvesVector3* center, float radius, SylvesError* error_out) {
    if (!center || radius <= 0) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    SphContext* ctx = (SphContext*)malloc(sizeof(SphContext));
    if (!ctx) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ctx->center = *center;
    ctx->radius = radius;
    SylvesDeformation* d = sylves_deformation_create(
        sph_map, NULL, NULL, NULL,
        ctx,
        false,
        error_out
    );
    if (!d) {
        free(ctx);
        return NULL;
    }
    return d;
}

SylvesDeformation* sylves_deformation_chain(SylvesDeformation* first, SylvesDeformation* second, SylvesError* error_out) {
    if (!first || !second) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    /* Compose by pre/post multiplying the identity is non-trivial with our API.
       For now, return a clone of second pre-multiplied by first's post matrix isn't correct without exposing matrices.
       Proper composition would need an API to compose deformations; omitted here to keep parity constraints. */
    if (error_out) *error_out = SYLVES_ERROR_NOT_IMPLEMENTED;
    return NULL;
}

SylvesDeformation* sylves_deformation_optimize(SylvesDeformation* deform, SylvesError* error_out) {
    (void)error_out;
    return deform;
}


